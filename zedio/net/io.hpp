#pragma once

#include "zedio/common/concepts.hpp"
#include "zedio/io/io.hpp"

namespace zedio::net::detail {

class SocketIO : public io::detail::IO {
private:
    explicit SocketIO(int fd)
        : IO{fd} {}

public:
    [[REMEMBER_CO_AWAIT]]
    auto shutdown(io::ShutdownBehavior how) noexcept {
        return io::detail::Shutdown{fd_, how};
    }

    [[REMEMBER_CO_AWAIT]]
    auto send(std::span<const char> buf) noexcept {
        return io::detail::Send{fd_, buf.data(), buf.size_bytes(), MSG_NOSIGNAL};
    }

    template <typename Addr>
        requires is_socket_address<Addr>
    [[REMEMBER_CO_AWAIT]]
    auto send_to(std::span<const char> buf, const Addr &addr) noexcept {
        return io::detail::SendTo(fd_,
                                  buf.data(),
                                  buf.size_bytes(),
                                  MSG_NOSIGNAL,
                                  addr.sockaddr(),
                                  addr.length());
    }

    [[REMEMBER_CO_AWAIT]]
    auto recv(std::span<char> buf, int flags = 0) const noexcept {
        return io::detail::Recv{fd_, buf.data(), buf.size_bytes(), flags};
    }

    template <typename Addr>
        requires is_socket_address<Addr>
    [[REMEMBER_CO_AWAIT]]
    auto recv_from(std::span<char> buf, unsigned flags = 0) const noexcept {
        class RecvFrom : public io::detail::IORegistrator<RecvFrom> {
        private:
            using Super = io::detail::IORegistrator<RecvFrom>;

        public:
            RecvFrom(int fd, std::span<char> buf, unsigned flags)
                : Super{io_uring_prep_recvmsg, fd, &msg_, flags}
                , iovecs_{.iov_base = buf.data(), .iov_len = buf.size_bytes()}
                , msg_{.msg_name = &addr_,
                       .msg_namelen = sizeof(addr_),
                       .msg_iov = &iovecs_,
                       .msg_iovlen = 1,
                       .msg_control = nullptr,
                       .msg_controllen = 0,
                       .msg_flags = 0} {};

            auto await_resume() const noexcept -> Result<std::pair<std::size_t, Addr>> {
                if (this->cb_.result_ >= 0) [[likely]] {
                    return std::make_pair(static_cast<std::size_t>(this->cb_.result_), addr_);
                } else {
                    return std::unexpected{make_sys_error(-this->cb_.result_)};
                }
            }

        private:
            struct iovec  iovecs_;
            Addr          addr_{};
            struct msghdr msg_ {};
        };
        return RecvFrom{fd_, buf, flags};
    }

    template <typename Addr>
        requires is_socket_address<Addr>
    [[REMEMBER_CO_AWAIT]]
    auto peek_from(std::span<char> buf) const noexcept {
        return recv_from<Addr>(buf, MSG_PEEK);
    }

    template <typename Stream, typename Addr>
        requires is_socket_address<Addr>
    [[REMEMBER_CO_AWAIT]]
    auto accept() const noexcept {
        class Accept : public io::detail::IORegistrator<Accept> {
            using Super = io::detail::IORegistrator<Accept>;

        public:
            Accept(int fd)
                : Super{io_uring_prep_accept,
                        fd,
                        reinterpret_cast<struct sockaddr *>(&addr_),
                        &length_,
                        SOCK_NONBLOCK} {}

            auto await_resume() const noexcept -> Result<std::pair<Stream, Addr>> {
                if (this->cb_.result_ >= 0) [[likely]] {
                    return std::make_pair(Stream{SocketIO{this->cb_.result_}}, addr_);
                } else {
                    return std::unexpected{make_sys_error(-this->cb_.result_)};
                }
            }

        private:
            Addr      addr_{};
            socklen_t length_{sizeof(Addr)};
        };
        return Accept{fd_};
    }

    template <typename Addr>
        requires is_socket_address<Addr>
    [[REMEMBER_CO_AWAIT]]
    auto connect(const Addr &addr) noexcept {
        return io::detail::Connect(fd_, addr.sockaddr(), addr.length());
    }

    template <typename Addr>
        requires is_socket_address<Addr>
    [[nodiscard]]
    auto bind(const Addr &addr) noexcept -> Result<void> {
        if (::bind(fd_, addr.sockaddr(), addr.length()) == -1) [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        return {};
    }

    [[nodiscard]]
    auto listen(int n) noexcept -> Result<void> {
        if (::listen(fd_, n) == -1) [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        return {};
    }

    template <typename Addr>
        requires is_socket_address<Addr>
    [[nodiscard]]
    auto local_addr() const noexcept -> Result<Addr> {
        Addr      addr{};
        socklen_t len{sizeof(addr)};
        if (::getsockname(fd_, addr.sockaddr(), &len) == -1) [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        return addr;
    }

    template <typename Addr>
        requires is_socket_address<Addr>
    [[nodiscard]]
    auto peer_addr() const noexcept -> Result<Addr> {
        Addr      addr{};
        socklen_t len{sizeof(addr)};
        if (::getpeername(fd_, addr.sockaddr(), &len) == -1) [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        return addr;
    }

    [[nodiscard]]
    auto set_reuseaddr(bool on) noexcept -> Result<void> {
        auto optval{on ? 1 : 0};
        return set_sock_opt(SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    }

    [[nodiscard]]
    auto reuseaddr() const noexcept -> Result<bool> {
        auto optval{0};
        if (auto ret = get_sock_opt(SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)); ret)
            [[likely]] {
            return optval != 0;
        } else {
            return std::unexpected{ret.error()};
        }
    }

    [[nodiscard]]
    auto set_reuseport(bool on) noexcept -> Result<void> {
        auto optval{on ? 1 : 0};
        return set_sock_opt(SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    }

    [[nodiscard]]
    auto reuseport() const noexcept -> Result<bool> {
        auto optval{0};
        if (auto ret = get_sock_opt(SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)); ret)
            [[likely]] {
            return optval != 0;
        } else {
            return std::unexpected{ret.error()};
        }
    }

    [[nodiscard]]
    auto set_ttl(uint32_t ttl) noexcept -> Result<void> {
        return set_sock_opt(IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));
    }

    [[nodiscard]]
    auto ttl() const noexcept -> Result<uint32_t> {
        uint32_t optval{0};
        if (auto ret = get_sock_opt(IPPROTO_IP, IP_TTL, &optval, sizeof(optval)); ret) [[likely]] {
            return optval;
        } else {
            return std::unexpected{ret.error()};
        }
    }

    [[nodiscard]]
    auto set_linger(std::optional<std::chrono::seconds> duration) noexcept -> Result<void> {
        struct linger lin {
            .l_onoff{0}, .l_linger{0},
        };
        if (duration.has_value()) {
            lin.l_onoff = 1;
            lin.l_linger = duration.value().count();
        }
        return set_sock_opt(SOL_SOCKET, SO_LINGER, &lin, sizeof(lin));
    }

    [[nodiscard]]
    auto linger() const noexcept -> Result<std::optional<std::chrono::seconds>> {
        struct linger lin;
        if (auto ret = get_sock_opt(SOL_SOCKET, SO_LINGER, &lin, sizeof(lin)); ret) [[likely]] {
            if (lin.l_onoff == 0) {
                return std::nullopt;
            } else {
                return std::chrono::seconds(lin.l_linger);
            }
        } else {
            return std::unexpected{ret.error()};
        }
    }

    [[nodiscard]]
    auto set_broadcast(bool on) noexcept -> Result<void> {
        auto optval{on ? 1 : 0};
        return set_sock_opt(SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));
    }

    [[nodiscard]]
    auto broadcast() const noexcept -> Result<bool> {
        auto optval{0};
        if (auto ret = get_sock_opt(SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)); ret)
            [[likely]] {
            return optval != 0;
        } else {
            return std::unexpected{ret.error()};
        }
    }

    [[nodiscard]]
    auto set_keepalive(bool on) noexcept {
        auto optval{on ? 1 : 0};
        return set_sock_opt(SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
    }

    [[nodiscard]]
    auto keepalive() const noexcept -> Result<bool> {
        auto optval{0};
        if (auto ret = get_sock_opt(SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)); ret)
            [[likely]] {
            return optval != 0;
        } else {
            return std::unexpected{ret.error()};
        }
    }

    [[nodiscard]]
    auto set_recv_buffer_size(int size) noexcept {
        return set_sock_opt(SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
    }

    [[nodiscard]]
    auto recv_buffer_size() const noexcept -> Result<std::size_t> {
        auto size{0};
        if (auto ret = get_sock_opt(SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)); ret) [[likely]] {
            return static_cast<std::size_t>(size);
        } else {
            return std::unexpected{ret.error()};
        }
    }

    [[nodiscard]]
    auto set_send_buffer_size(int size) noexcept {
        return set_sock_opt(SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
    }

    [[nodiscard]]
    auto send_buffer_size() const noexcept -> Result<std::size_t> {
        auto size{0};
        if (auto ret = get_sock_opt(SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)); ret) [[likely]] {
            return static_cast<std::size_t>(size);
        } else {
            return std::unexpected{ret.error()};
        }
    }

    [[nodiscard]]
    auto set_mark(uint32_t mark) noexcept {
        return set_sock_opt(SOL_SOCKET, SO_MARK, &mark, sizeof(mark));
    }

    [[nodiscard]]
    auto set_passcred(bool on) noexcept {
        int optval{on ? 1 : 0};
        return set_sock_opt(SOL_SOCKET, SO_PASSCRED, &optval, sizeof(optval));
    }

    [[nodiscard]]
    auto passcred() const noexcept -> Result<bool> {
        int optval{0};
        if (auto ret = get_sock_opt(SOL_SOCKET, SO_PASSCRED, &optval, sizeof(optval)); ret)
            [[likely]] {
            return optval != 0;
        } else {
            return std::unexpected{ret.error()};
        }
    }

public:
    [[nodiscard]]
    static auto build_socket(int domain, int type, int protocol) -> Result<SocketIO> {
        if (auto fd = ::socket(domain, type | SOCK_NONBLOCK, protocol); fd != -1) [[likely]] {
            return SocketIO{fd};
        } else {
            return std::unexpected{make_sys_error(errno)};
        }
    }

    template <typename Stream, typename Addr>
        requires is_socket_address<Addr>
    [[REMEMBER_CO_AWAIT]]
    static auto build_stream(const Addr &addr) {
        class Connect : public io::detail::IORegistrator<Connect> {
        private:
            using Super = io::detail::IORegistrator<Connect>;

        public:
            Connect(const Addr &addr)
                : Super{io_uring_prep_connect, -1, nullptr, sizeof(Addr)}
                , addr_{addr} {}

            auto await_suspend(std::coroutine_handle<> handle) -> bool {
                if (auto ret = SocketIO::build_socket(addr_.family(), SOCK_STREAM, 0); !ret)
                    [[unlikely]] {
                    this->cb_.result_ = ret.error().value();
                    io_uring_prep_nop(this->sqe_);
                    io_uring_sqe_set_data(this->sqe_, nullptr);
                    return false;
                } else {
                    io_ = std::move(ret.value());
                }
                this->sqe_->fd = io_.fd();
                this->sqe_->addr = (unsigned long)addr_.sockaddr();
                Super::await_suspend(handle);
                return true;
            }

            auto await_resume() noexcept -> Result<Stream> {
                if (this->cb_.result_ >= 0) [[likely]] {
                    return Stream{std::move(io_)};
                } else {
                    return std::unexpected{make_sys_error(-this->cb_.result_)};
                }
            }

        private:
            SocketIO io_{-1};
            Addr     addr_;
        };
        return Connect(addr);
    }

    template <typename Listener, typename Addr>
        requires is_socket_address<Addr>
    [[REMEMBER_CO_AWAIT]]
    static auto build_listener(const Addr &addr) -> Result<Listener> {
        auto io = SocketIO::build_socket(addr.family(), SOCK_STREAM, 0);
        if (!io) [[unlikely]] {
            return std::unexpected{io.error()};
        }
        Result<void> ret{};
        if (ret = io.value().bind(addr); !ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        if (ret = io.value().listen(SOMAXCONN); !ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        return Listener{std::move(io.value())};
    }

private:
    [[nodiscard]]
    auto set_sock_opt(int level, int optname, const void *optval, socklen_t optlen) noexcept
        -> Result<void> {
        if (::setsockopt(fd_, level, optname, optval, optlen) == -1) [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        return {};
    }

    [[nodiscard]]
    auto get_sock_opt(int level, int optname, void *optval, socklen_t optlen) const noexcept
        -> Result<void> {
        if (auto ret = ::getsockopt(fd_, level, optname, optval, &optlen); ret == -1) [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        return {};
    }
};

} // namespace zedio::net::detail
