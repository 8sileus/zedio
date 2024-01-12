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
#include <system_error>

namespace zed::async::detail {

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] BaseIOAwaiter {
    BaseIOAwaiter()
        : sqe_{io_uring_get_sqe(t_poller->ring())}
        , data_{static_cast<int>(flag)} {}

    auto await_ready() const noexcept -> bool {
        return sqe_ == nullptr;
    }

    auto await_suspend(std::coroutine_handle<> handle) -> std::coroutine_handle<> {
        assert(sqe_ != nullptr);

        data_.handle_ = std::move(handle);

        if constexpr (flag == OPFlag::Registered) {
            sqe_->flags |= IOSQE_FIXED_FILE;
        }

        io_uring_sqe_set_data(sqe_, &data_);
        if (data_.result_ = io_uring_submit(t_poller->ring()); data_.result_ < 0) [[unlikely]] {
            return handle;
        }
        return std::noop_coroutine();
    }

    constexpr auto await_resume() const noexcept -> std::expected<int, std::error_code> {
        if (data_.result_ >= 0) [[likely]] {
            return data_.result_;
        }
        if (sqe_ == nullptr) [[unlikely]] {
            return std::unexpected{
                std::error_code{static_cast<int>(Error::Nosqe), zed_category()}
            };
        }
        return std::unexpected{
            std::error_code{-data_.result_, std::system_category()}
        };
    }

protected:
    io_uring_sqe     *sqe_;
    BaseIOAwaiterData data_;
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] SocketAwaiter : public BaseIOAwaiter<flag> {
    SocketAwaiter(int domain, int type, int protocol, unsigned int flags) {
        if (this->sqe_) [[likely]] {
            type |= SOCK_NONBLOCK;
            io_uring_prep_socket(this->sqe_, domain, type, protocol, flags);
        }
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] AcceptAwaiter : public BaseIOAwaiter<flag> {
    AcceptAwaiter(int fd, sockaddr *addr, socklen_t *addrlen) {
        if (this->sqe_) [[likely]] {
            io_uring_prep_accept(this->sqe_, fd, addr, addrlen, SOCK_NONBLOCK);
        }
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] ConnectAwaiter : public BaseIOAwaiter<flag> {
    ConnectAwaiter(int fd, const sockaddr *addr, socklen_t addrlen) {
        if (this->sqe_) [[likely]] {
            io_uring_prep_connect(this->sqe_, fd, addr, addrlen);
        }
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] ShutdownAwaiter : public BaseIOAwaiter<flag> {
    ShutdownAwaiter(int fd, int how) {
        if (this->sqe_) [[likely]] {
            io_uring_prep_shutdown(this->sqe_, fd, how);
        }
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] CloseAwaiter : public BaseIOAwaiter<flag> {
    CloseAwaiter(int fd) {
        if (this->sqe_) [[likely]] {
            io_uring_prep_close(this->sqe_, fd);
        }
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] RecvAwaiter : public BaseIOAwaiter<flag> {
    RecvAwaiter(int sockfd, void *buf, std::size_t len, int flags) {
        if (this->sqe_) [[likely]] {
            io_uring_prep_recv(this->sqe_, sockfd, buf, len, flags);
        }
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] RecvMsgAwaiter : public BaseIOAwaiter<flag> {
    RecvMsgAwaiter(int fd, msghdr *msg, unsigned flags) {
        if (this->sqe_) [[likely]] {
            io_uring_prep_recvmsg(this->sqe_, fd, msg, flags);
        }
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] SendAwaiter : public BaseIOAwaiter<flag> {
    SendAwaiter(int sockfd, const void *buf, std::size_t len, int flags) {
        if (this->sqe_) [[likely]] {
            io_uring_prep_send(this->sqe_, sockfd, buf, len, flags);
        }
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] SendMsgAwaiter : public BaseIOAwaiter<flag> {
    SendMsgAwaiter(int fd, const msghdr *msg, unsigned flags) {
        if (this->sqe_) [[likely]] {
            io_uring_prep_sendmsg(this->sqe_, fd, msg, flags);
        }
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] SendToAwaiter : public BaseIOAwaiter<flag> {
    SendToAwaiter(int sockfd, const void *buf, size_t len, int flags, const sockaddr *addr,
                  socklen_t addrlen) {
        if (this->sqe_) [[likely]] {
            io_uring_prep_sendto(this->sqe_, sockfd, buf, len, flags, addr, addrlen);
        }
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] ReadAwaiter : public BaseIOAwaiter<flag> {
    ReadAwaiter(int fd, void *buf, std::size_t nbytes, uint64_t offset) {
        if (this->sqe_) [[likely]] {
            io_uring_prep_read(this->sqe_, fd, buf, nbytes, offset);
        }
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] ReadvAwaiter : public BaseIOAwaiter<flag> {
    ReadvAwaiter(int fd, const iovec *iovecs, int nr_vecs, uint64_t offset) {
        if (this->sqe_) [[likely]] {
            io_uring_prep_readv(this->sqe_, fd, iovecs, nr_vecs, offset);
        }
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] Readv2Awaiter : public BaseIOAwaiter<flag> {
    Readv2Awaiter(int fd, const iovec *iovecs, int nr_vecs, uint64_t offset, int flags) {
        if (this->sqe_) [[likely]] {
            io_uring_prep_readv2(this->sqe_, fd, iovecs, nr_vecs, offset, flags);
        }
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] WriteAwaiter : public BaseIOAwaiter<flag> {
    WriteAwaiter(int fd, const void *buf, unsigned nbytes, uint64_t offset) {
        if (this->sqe_) [[likely]] {
            io_uring_prep_write(this->sqe_, fd, buf, nbytes, offset);
        }
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] WritevAwaiter : public BaseIOAwaiter<flag> {
    WritevAwaiter(int fd, const iovec *iovecs, unsigned nr_vecs, uint64_t offset) {
        if (this->sqe_) [[likely]] {
            io_uring_prep_writev(this->sqe_, fd, iovecs, nr_vecs, offset);
        }
    }
};

template <OPFlag flag>
struct [[REMEMBER_CO_AWAIT]] Writev2Awaiter : public BaseIOAwaiter<flag> {
    Writev2Awaiter(int fd, const iovec *iovecs, unsigned nr_vecs, uint64_t offset, int flags) {
        if (this->sqe_) [[likely]] {
            io_uring_prep_writev2(this->sqe_, fd, iovecs, nr_vecs, offset, flags);
        }
    }
};

} // namespace zed::async::detail
