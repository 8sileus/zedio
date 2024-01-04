#pragma once

#include "common/error.hpp"
#include "common/macros.hpp"
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

struct [[NODISCARD_CO_AWAIT]] BaseIOAwaiter {
    BaseIOAwaiter(uint32_t state);

    auto await_ready() const noexcept -> bool {
        return (state_ & READY) != 0;
    }

    void await_suspend(std::coroutine_handle<> handle);

    constexpr auto await_resume() const noexcept -> std::expected<int, std::error_code> {
        if (result_ >= 0) [[likely]] {
            return result_;
        }
        if (sqe_ == nullptr) [[unlikely]] {
            return std::unexpected{
                std::error_code{static_cast<int>(Error::Nosqe), zed_category()}
            };
        }
        return std::unexpected{
            std::error_code{-result_, std::system_category()}
        };
    }

    [[nodiscard]]
    auto is_distributable() const -> bool {
        return (state_ & DISTRIBUTABLE) != 0;
    }

    void set_result(int result) {
        result_ = result;
    }

    [[nodiscard]]
    auto handle() -> std::coroutine_handle<> & {
        return handle_;
    }

protected:
    static constexpr uint32_t READY{1};
    static constexpr uint32_t DISTRIBUTABLE{1 << 1};

protected:
    io_uring_sqe           *sqe_;
    std::coroutine_handle<> handle_;
    uint32_t                state_;
    int                     result_{-1};
};

enum class AccessLevel {
    Exclusive = 0,
    Distributive = 1 << 1,
};

template <AccessLevel level>
struct [[NODISCARD_CO_AWAIT]] SocketAwaiter : public BaseIOAwaiter {
    SocketAwaiter(int domain, int type, int protocol, unsigned int flags)
        : BaseIOAwaiter{static_cast<uint32_t>(level)} {
        type |= SOCK_NONBLOCK;
        io_uring_prep_socket(sqe_, domain, type, protocol, flags);
    }
};

template <AccessLevel level>
struct [[NODISCARD_CO_AWAIT]] AcceptAwaiter : public BaseIOAwaiter {
    AcceptAwaiter(int fd, sockaddr *addr, socklen_t *addrlen)
        : BaseIOAwaiter{static_cast<uint32_t>(level)} {
        if (sqe_) [[likely]] {
            io_uring_prep_accept(sqe_, fd, addr, addrlen, SOCK_NONBLOCK);
        }
    }
};

template <AccessLevel level>
struct [[NODISCARD_CO_AWAIT]] ConnectAwaiter : public BaseIOAwaiter {
    ConnectAwaiter(int fd, const sockaddr *addr, socklen_t addrlen)
        : BaseIOAwaiter{static_cast<uint32_t>(level)} {
        if (sqe_) [[likely]] {
            io_uring_prep_connect(sqe_, fd, addr, addrlen);
        }
    }
};

template <AccessLevel level>
struct [[NODISCARD_CO_AWAIT]] ShutdownAwaiter : public BaseIOAwaiter {
    ShutdownAwaiter(int fd, int how)
        : BaseIOAwaiter{static_cast<uint32_t>(level)} {
        if (sqe_) [[likely]] {
            io_uring_prep_shutdown(sqe_, fd, how);
        }
    }
};

template <AccessLevel level>
struct [[NODISCARD_CO_AWAIT]] CloseAwaiter : public BaseIOAwaiter {
    CloseAwaiter(int fd)
        : BaseIOAwaiter{static_cast<uint32_t>(level)} {
        if (sqe_) [[likely]] {
            io_uring_prep_close(sqe_, fd);
        }
    }
};

template <AccessLevel level>
struct [[NODISCARD_CO_AWAIT]] RecvAwaiter : public BaseIOAwaiter {
    RecvAwaiter(int sockfd, void *buf, std::size_t len, int flags)
        : BaseIOAwaiter{static_cast<uint32_t>(level)} {
        if (sqe_) [[likely]] {
            io_uring_prep_recv(sqe_, sockfd, buf, len, flags);
        }
    }
};

template <AccessLevel level>
struct [[NODISCARD_CO_AWAIT]] RecvMsgAwaiter : public BaseIOAwaiter {
    RecvMsgAwaiter(int fd, msghdr *msg, unsigned flags)
        : BaseIOAwaiter{static_cast<uint32_t>(level)} {
        if (sqe_) [[likely]] {
            io_uring_prep_recvmsg(sqe_, fd, msg, flags);
        }
    }
};

template <AccessLevel level>
struct [[NODISCARD_CO_AWAIT]] SendAwaiter : public BaseIOAwaiter {
    SendAwaiter(int sockfd, const void *buf, std::size_t len, int flags)
        : BaseIOAwaiter{static_cast<uint32_t>(level)} {
        if (sqe_) [[likely]] {
            io_uring_prep_send(sqe_, sockfd, buf, len, flags);
        }
    }
};

template <AccessLevel level>
struct [[NODISCARD_CO_AWAIT]] SendMsgAwaiter : public BaseIOAwaiter {
    SendMsgAwaiter(int fd, const msghdr *msg, unsigned flags)
        : BaseIOAwaiter{static_cast<uint32_t>(level)} {
        if (sqe_) [[likely]] {
            io_uring_prep_sendmsg(sqe_, fd, msg, flags);
        }
    }
};

template <AccessLevel level>
struct [[NODISCARD_CO_AWAIT]] SendToAwaiter : public BaseIOAwaiter {
    SendToAwaiter(int sockfd, const void *buf, size_t len, int flags, const sockaddr *addr,
                  socklen_t addrlen)
        : BaseIOAwaiter{static_cast<uint32_t>(level)} {
        if (sqe_) [[likely]] {
            io_uring_prep_sendto(sqe_, sockfd, buf, len, flags, addr, addrlen);
        }
    }
};

template <AccessLevel level>
struct [[NODISCARD_CO_AWAIT]] ReadAwaiter : public BaseIOAwaiter {
    ReadAwaiter(int fd, void *buf, std::size_t nbytes, uint64_t offset)
        : BaseIOAwaiter{static_cast<uint32_t>(level)} {
        if (sqe_) [[likely]] {
            io_uring_prep_read(sqe_, fd, buf, nbytes, offset);
        }
    }
};

template <AccessLevel level>
struct [[NODISCARD_CO_AWAIT]] ReadvAwaiter : public BaseIOAwaiter {
    ReadvAwaiter(int fd, const iovec *iovecs, int nr_vecs, uint64_t offset)
        : BaseIOAwaiter{static_cast<uint32_t>(level)} {
        if (sqe_) [[likely]] {
            io_uring_prep_readv(sqe_, fd, iovecs, nr_vecs, offset);
        }
    }
};

template <AccessLevel level>
struct [[NODISCARD_CO_AWAIT]] Readv2Awaiter : public BaseIOAwaiter {
    Readv2Awaiter(int fd, const iovec *iovecs, int nr_vecs, uint64_t offset, int flags)
        : BaseIOAwaiter{static_cast<uint32_t>(level)} {
        if (sqe_) [[likely]] {
            io_uring_prep_readv2(sqe_, fd, iovecs, nr_vecs, offset, flags);
        }
    }
};

template <AccessLevel level>
struct [[NODISCARD_CO_AWAIT]] WriteAwaiter : public BaseIOAwaiter {
    WriteAwaiter(int fd, const void *buf, unsigned nbytes, uint64_t offset)
        : BaseIOAwaiter{static_cast<uint32_t>(level)} {
        if (sqe_) [[likely]] {
            io_uring_prep_write(sqe_, fd, buf, nbytes, offset);
        }
    }
};

template <AccessLevel level>
struct [[NODISCARD_CO_AWAIT]] WritevAwaiter : public BaseIOAwaiter {
    WritevAwaiter(int fd, const iovec *iovecs, unsigned nr_vecs, uint64_t offset)
        : BaseIOAwaiter{static_cast<uint32_t>(level)} {
        if (sqe_) [[likely]] {
            io_uring_prep_writev(sqe_, fd, iovecs, nr_vecs, offset);
        }
    }
};

template <AccessLevel level>
struct [[NODISCARD_CO_AWAIT]] Writev2Awaiter : public BaseIOAwaiter {
    Writev2Awaiter(int fd, const iovec *iovecs, unsigned nr_vecs, uint64_t offset, int flags)
        : BaseIOAwaiter{static_cast<uint32_t>(level)} {
        if (sqe_) [[likely]] {
            io_uring_prep_writev2(sqe_, fd, iovecs, nr_vecs, offset, flags);
        }
    }
};

} // namespace zed::async::detail
