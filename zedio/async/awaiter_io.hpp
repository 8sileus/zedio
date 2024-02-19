#pragma once

#include "zedio/async/poller.hpp"
#include "zedio/common/error.hpp"
#include "zedio/common/macros.hpp"
// Linux
#include <liburing.h>
// C
#include <cassert>
// C++
#include <coroutine>
#include <expected>
#include <functional>

namespace zedio::async::detail {

template <typename ReturnType>
struct [[REMEMBER_CO_AWAIT]] BaseIOAwaiter {
    BaseIOAwaiter(int state)
        : data_{state} {}

    // Delete copy
    BaseIOAwaiter(const BaseIOAwaiter &) = delete;
    auto operator=(const BaseIOAwaiter &) -> BaseIOAwaiter & = delete;

    // Delete move
    BaseIOAwaiter(BaseIOAwaiter &&) = delete;
    auto operator=(BaseIOAwaiter &&) -> BaseIOAwaiter & = delete;

    auto await_ready() const noexcept -> bool {
        return data_.is_ready();
    }

    auto await_suspend(std::coroutine_handle<> handle) -> std::coroutine_handle<> {
        data_.handle_ = std::move(handle);
        io_uring_sqe_set_data(sqe_, &this->data_);
        if (data_.result_ = io_uring_submit(t_poller->ring()); data_.result_ < 0) [[unlikely]] {
            return handle;
        }
        return std::noop_coroutine();
    }

    auto await_resume() const noexcept -> Result<ReturnType> {
        if (data_.is_good_result()) [[likely]] {
            if constexpr (std::is_same_v<void, ReturnType>) {
                return {};
            } else {
                return static_cast<ReturnType>(data_.result_);
            }
        }
        if (data_.is_bad_result()) {
            return std::unexpected{make_sys_error(-data_.result_)};
        }
        assert(data_.is_null_sqe());
        return std::unexpected{make_zedio_error(Error::NullSeq)};
    }

protected:
    io_uring_sqe     *sqe_{nullptr};
    BaseIOAwaiterData data_;
};

#define REGISTER_IO(f, ...)                          \
    this->sqe_ = io_uring_get_sqe(t_poller->ring()); \
    if (this->sqe_ != nullptr) [[likely]] {          \
        f(this->sqe_, __VA_ARGS__);                  \
        if constexpr (flag == OPFlag::Registered) {  \
            this->sqe_->flags |= IOSQE_FIXED_FILE;   \
        }                                            \
    } else {                                         \
        this->data_.set_null_sqe();                  \
    }

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] SocketAwaiter : public BaseIOAwaiter<int> {
    SocketAwaiter(int domain, int type, int protocol, unsigned int flags)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        REGISTER_IO(io_uring_prep_socket, domain, type, protocol, flags);
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] AcceptAwaiter : public BaseIOAwaiter<int> {
    AcceptAwaiter(int fd, sockaddr *addr, socklen_t *addrlen)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        REGISTER_IO(io_uring_prep_accept, fd, addr, addrlen, SOCK_NONBLOCK)
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] ConnectAwaiter : public BaseIOAwaiter<void> {
    ConnectAwaiter(int fd, const sockaddr *addr, socklen_t addrlen)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        REGISTER_IO(io_uring_prep_connect, fd, addr, addrlen)
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] ShutdownAwaiter : public BaseIOAwaiter<void> {
    ShutdownAwaiter(int fd, int how)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        REGISTER_IO(io_uring_prep_shutdown, fd, how)
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] CloseAwaiter : public BaseIOAwaiter<void> {
    CloseAwaiter(int fd)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        REGISTER_IO(io_uring_prep_close, fd)
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] RecvAwaiter : public BaseIOAwaiter<std::size_t> {
    RecvAwaiter(int sockfd, void *buf, std::size_t len, int flags)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        REGISTER_IO(io_uring_prep_recv, sockfd, buf, len, flags)
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] RecvMsgAwaiter : public BaseIOAwaiter<std::size_t> {
    RecvMsgAwaiter(int fd, msghdr *msg, unsigned flags)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        REGISTER_IO(io_uring_prep_recvmsg, fd, msg, flags)
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] SendAwaiter : public BaseIOAwaiter<std::size_t> {
    SendAwaiter(int sockfd, const void *buf, std::size_t len, int flags)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        REGISTER_IO(io_uring_prep_send, sockfd, buf, len, flags)
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] SendMsgAwaiter : public BaseIOAwaiter<std::size_t> {
    SendMsgAwaiter(int fd, const msghdr *msg, unsigned flags)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        REGISTER_IO(io_uring_prep_sendmsg, fd, msg, flags)
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] SendToAwaiter : public BaseIOAwaiter<std::size_t> {
    SendToAwaiter(int             sockfd,
                  const void     *buf,
                  size_t          len,
                  int             flags,
                  const sockaddr *addr,
                  socklen_t       addrlen)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        REGISTER_IO(io_uring_prep_sendto, sockfd, buf, len, flags, addr, addrlen)
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] ReadAwaiter : public BaseIOAwaiter<std::size_t> {
    ReadAwaiter(int fd, void *buf, std::size_t nbytes, uint64_t offset)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        REGISTER_IO(io_uring_prep_read, fd, buf, nbytes, offset)
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] ReadvAwaiter : public BaseIOAwaiter<std::size_t> {
    ReadvAwaiter(int fd, iovec *iovecs, int nr_vecs, uint64_t offset)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        REGISTER_IO(io_uring_prep_readv, fd, iovecs, nr_vecs, offset)
    }
};

// template <OPFlag flag>
// struct [[REMEMBER_CO_AWAIT]] Readv2Awaiter : public BaseIOAwaiter {
//     Readv2Awaiter(int fd, iovec *iovecs, int nr_vecs, uint64_t offset, int flags) {
//         if (this->sqe_) [[likely]] {
//             io_uring_prep_readv2(this->sqe_, fd, iovecs, nr_vecs, offset, flags);
//         }
//     }
// };

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] WriteAwaiter : public BaseIOAwaiter<std::size_t> {
    WriteAwaiter(int fd, const void *buf, unsigned nbytes, uint64_t offset)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        REGISTER_IO(io_uring_prep_write, fd, buf, nbytes, offset)
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] WritevAwaiter : public BaseIOAwaiter<std::size_t> {
    WritevAwaiter(int fd, const iovec *iovecs, unsigned nr_vecs, uint64_t offset)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        REGISTER_IO(io_uring_prep_writev, fd, iovecs, nr_vecs, offset)
    }
};

// template <OPFlag flag>
// struct [[REMEMBER_CO_AWAIT]] Writev2Awaiter : public BaseIOAwaiter {
//     Writev2Awaiter(int fd, const iovec *iovecs, unsigned nr_vecs, uint64_t offset, int flags) {
//         if (this->sqe_) [[likely]] {
//             io_uring_prep_writev2(this->sqe_, fd, iovecs, nr_vecs, offset, flags);
//         }
//     }
// };

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] OpenAt2Awaiter : public BaseIOAwaiter<int> {
    OpenAt2Awaiter(int dfd, const char *path, struct open_how *how)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        REGISTER_IO(io_uring_prep_openat2, dfd, path, how)
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] OpenAtAwaiter : public BaseIOAwaiter<int> {
    OpenAtAwaiter(int dfd, const char *path, int flags, mode_t mode)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        REGISTER_IO(io_uring_prep_openat, dfd, path, flags, mode)
    }
};

} // namespace zedio::async::detail

#undef REGISTER_IO