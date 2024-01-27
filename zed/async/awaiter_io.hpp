#pragma once

#include "zed/async/poller.hpp"
#include "zed/common/error.hpp"
#include "zed/common/macros.hpp"
// Linux
#include <liburing.h>
// C
#include <cassert>
// C++
#include <coroutine>
#include <expected>
#include <functional>

namespace zed::async::detail {

struct [[REMEMBER_CO_AWAIT]] BaseIOAwaiter {
    BaseIOAwaiter(int state)
        : data_{state} {}

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

    constexpr auto await_resume() const noexcept -> std::expected<int, std::error_code> {
        if (data_.is_good_result()) [[likely]] {
            return data_.result_;
        }
        if (data_.is_bad_result()) {
            return std::unexpected{make_sys_error(-data_.result_)};
        }
        assert(data_.is_null_sqe());
        return std::unexpected{make_zed_error(Error::Nosqe)};
    }

protected:
    io_uring_sqe     *sqe_{nullptr};
    BaseIOAwaiterData data_;
};

#define EXECUTE_IO(f, ...)                                           \
    if constexpr (flag != OPFlag::Registered) {                      \
        auto ret = f(__VA_ARGS__);                                   \
        if (ret != -1) {                                             \
            this->data_.set_result(ret);                             \
            return;                                                  \
        } else if (errno != EAGAIN && errno != EINTR) [[unlikely]] { \
            this->data_.set_result(-errno);                          \
            return;                                                  \
        }                                                            \
    }

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
struct [[REMEMBER_CO_AWAIT]] SocketAwaiter : public BaseIOAwaiter {
    SocketAwaiter(int domain, int type, int protocol, unsigned int flags)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        REGISTER_IO(io_uring_prep_socket, domain, type, protocol, flags);
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] AcceptAwaiter : public BaseIOAwaiter {
    AcceptAwaiter(int fd, sockaddr *addr, socklen_t *addrlen)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        EXECUTE_IO(::accept4, fd, addr, addrlen, SOCK_NONBLOCK)
        REGISTER_IO(io_uring_prep_accept, fd, addr, addrlen, SOCK_NONBLOCK)
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] ConnectAwaiter : public BaseIOAwaiter {
    ConnectAwaiter(int fd, const sockaddr *addr, socklen_t addrlen)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        REGISTER_IO(io_uring_prep_connect, fd, addr, addrlen)
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] ShutdownAwaiter : public BaseIOAwaiter {
    ShutdownAwaiter(int fd, int how)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        EXECUTE_IO(::shutdown, fd, how);
        REGISTER_IO(io_uring_prep_shutdown, fd, how)
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] CloseAwaiter : public BaseIOAwaiter {
    CloseAwaiter(int fd)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        EXECUTE_IO(::close, fd)
        REGISTER_IO(io_uring_prep_close, fd)
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] RecvAwaiter : public BaseIOAwaiter {
    RecvAwaiter(int sockfd, void *buf, std::size_t len, int flags)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        EXECUTE_IO(::recv, sockfd, buf, len, flags)
        REGISTER_IO(io_uring_prep_recv, sockfd, buf, len, flags)
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] RecvMsgAwaiter : public BaseIOAwaiter {
    RecvMsgAwaiter(int fd, msghdr *msg, unsigned flags)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        EXECUTE_IO(::recvmsg, fd, msg, flags)
        REGISTER_IO(io_uring_prep_recvmsg, fd, msg, flags)
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] SendAwaiter : public BaseIOAwaiter {
    SendAwaiter(int sockfd, const void *buf, std::size_t len, int flags)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        EXECUTE_IO(::send, sockfd, buf, len, flags)
        REGISTER_IO(io_uring_prep_send, sockfd, buf, len, flags)
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] SendMsgAwaiter : public BaseIOAwaiter {
    SendMsgAwaiter(int fd, const msghdr *msg, unsigned flags)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        EXECUTE_IO(::sendmsg, fd, msg, flags)
        REGISTER_IO(io_uring_prep_sendmsg, fd, msg, flags)
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] SendToAwaiter : public BaseIOAwaiter {
    SendToAwaiter(int sockfd, const void *buf, size_t len, int flags, const sockaddr *addr,
                  socklen_t addrlen)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        EXECUTE_IO(::sendto, sockfd, buf, len, flags, addr, addrlen)
        REGISTER_IO(io_uring_prep_sendto, sockfd, buf, len, flags, addr, addrlen)
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] ReadAwaiter : public BaseIOAwaiter {
    ReadAwaiter(int fd, void *buf, std::size_t nbytes, uint64_t offset)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        EXECUTE_IO(::read, fd, buf, nbytes)
        REGISTER_IO(io_uring_prep_read, fd, buf, nbytes, offset)
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] ReadvAwaiter : public BaseIOAwaiter {
    ReadvAwaiter(int fd, iovec *iovecs, int nr_vecs, uint64_t offset)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        EXECUTE_IO(::readv, fd, iovecs, nr_vecs)
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
struct [[REMEMBER_CO_AWAIT]] WriteAwaiter : public BaseIOAwaiter {
    WriteAwaiter(int fd, const void *buf, unsigned nbytes, uint64_t offset)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        EXECUTE_IO(::write, fd, buf, nbytes)
        REGISTER_IO(io_uring_prep_write, fd, buf, nbytes, offset)
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] WritevAwaiter : public BaseIOAwaiter {
    WritevAwaiter(int fd, const iovec *iovecs, unsigned nr_vecs, uint64_t offset)
        : BaseIOAwaiter(static_cast<int>(flag)) {
        EXECUTE_IO(::writev, fd, iovecs, nr_vecs)
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

} // namespace zed::async::detail

#undef EXECUTE_IO
#undef REGISTER_IO