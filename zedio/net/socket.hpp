#pragma once

#include "zedio/net/io.hpp"

namespace zedio::net::detail {

template <class Listener, class Stream, class Addr>
class BaseSocket {
protected:
    explicit BaseSocket(SocketIO &&io)
        : io_{std::move(io)} {}

public:
    [[nodiscard]]
    auto bind(const Addr &address) noexcept {
        return io_.bind<Addr>(address);
    }

    [[nodiscard]]
    auto fd() const noexcept {
        return io_.fd();
    }

    // Converts the socket into TcpListener
    [[nodiscard]]
    auto listen(int n) -> Result<Listener> {
        if (auto ret = io_.listen(n); !ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        return Listener{std::move(io_)};
    }

    // Converts the socket into TcpStream
    [[nodiscard]]
    auto connect(const Addr &addr) {
        class Awaiter : public io::detail::IORegistrator<Awaiter> {
            using Super = io::detail::IORegistrator<Awaiter>;

        public:
            Awaiter(detail::SocketIO &&io, const Addr &addr)
                : Super{io_uring_prep_connect, io.fd(), addr_.sockaddr(), addr.length()}
                , io_{std::move(io)}
                , addr_{addr} {}

            auto await_resume() const noexcept -> Result<Stream> {
                if (this->cb_.result_ >= 0) [[likely]] {
                    return Stream{std::move(io_)};
                } else {
                    return std::unexpected{make_sys_error(-this->cb_.result_)};
                }
            }

        private:
            detail::SocketIO io_;
            Addr             addr_;
        };
        return Awaiter{std::move(io_), addr};
    }

protected:
    SocketIO io_;
};

} // namespace zedio::net::detail
