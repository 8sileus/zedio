#pragma once

#include "zedio/io/io.hpp"
#include "zedio/socket/impl/impl_sockopt.hpp"

namespace zedio::socket::detail {

template <class Listener, class Stream, class Datagram, class Addr>
class BaseSocket : public io::detail::Fd {
protected:
    explicit BaseSocket(const int fd)
        : Fd{fd} {}

public:
    [[nodiscard]]
    auto bind(const Addr &addr) noexcept -> Result<void> {
        if (::bind(fd_, addr.sockaddr(), addr.length()) == -1) [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        return {};
    }

    // Converts the socket into TcpListener
    [[nodiscard]]
    auto listen(int n) -> Result<Listener> {
        if (socket_type().value() != SOCK_STREAM) {
            return std::unexpected{make_zedio_error(Error::InvalidSocketType)};
        }
        if (::listen(fd_, n) == -1) [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        return Listener{this->take_fd()};
    }

    // Converts the socket into TcpStream
    [[REMEMBER_CO_AWAIT]]
    auto connect(const Addr &addr) {
        class Awaiter : public io::detail::IORegistrator<Awaiter> {
            using Super = io::detail::IORegistrator<Awaiter>;

        public:
            Awaiter(const int fd, const Addr &addr)
                : Super{io_uring_prep_connect, fd, addr_.sockaddr(), addr.length()}
                , fd_{fd}
                , addr_{addr} {}

            auto await_resume() const noexcept -> Result<Stream> {
                if (this->cb_.result_ >= 0) [[likely]] {
                    return Stream{fd_};
                } else {
                    return std::unexpected{make_sys_error(-this->cb_.result_)};
                }
            }

        private:
            int  fd_;
            Addr addr_;
        };
        return Awaiter{this->take_fd(), addr};
    }

    // Converts the socket into DdpDatagram
    [[nodiscard]]
    auto datagram() -> Result<Datagram> {
        if (socket_type().value() != SOCK_DGRAM) {
            return std::unexpected{make_zedio_error(Error::InvalidSocketType)};
        }
        return Datagram{this->take_fd()};
    }

    [[nodiscard]]
    auto socket_type() -> Result<int> {
        int optval{0};
        if (auto ret = get_sock_opt(this->fd(), SOL_SOCKET, SO_TYPE, &optval, sizeof(optval)); !ret)
            [[unlikely]] {
            return std::unexpected{ret.error()};
        } else {
            return optval;
        }
    }
};

} // namespace zedio::socket::detail
