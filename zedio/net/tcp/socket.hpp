#pragma once

#include "zedio/common/error.hpp"
#include "zedio/net/tcp/listener.hpp"
#include "zedio/net/tcp/stream.hpp"
// C++
#include <expected>

namespace zedio::net {

class TcpSocket : util::Noncopyable {
    explicit TcpSocket(Socket &&sock)
        : io_{std::move(sock)} {}

public:
    TcpSocket(TcpSocket &&other)
        : io_{std::move(other.io_)} {}

    [[nodiscard]]
    auto bind(const SocketAddr &address) const noexcept {
        return io_.bind(address);
    }

    [[nodiscard]]
    auto fd() const noexcept {
        return io_.fd();
    }

    [[nodiscard]]
    auto set_reuseaddr(bool on) const noexcept {
        return io_.set_reuseaddr(on);
    }

    [[nodiscard]]
    auto reuseaddr() const noexcept {
        return io_.reuseaddr();
    }

    [[nodiscard]]
    auto set_reuseport(bool on) const noexcept {
        return io_.set_reuseport(on);
    }

    [[nodiscard]]
    auto reuseport() const noexcept {
        return io_.reuseport();
    }

    [[nodiscard]]
    auto set_nonblocking(bool on) const noexcept {
        return io_.set_nonblocking(on);
    }

    [[nodiscard]]
    auto nonblocking() const noexcept {
        return io_.nonblocking();
    }

    [[nodiscard]]
    auto set_nodelay(bool on) const noexcept {
        return io_.set_nodelay(on);
    }

    [[nodiscard]]
    auto nodelay() const noexcept {
        return io_.nodelay();
    }

    [[nodiscard]]
    auto set_linger(std::optional<std::chrono::seconds> duration) const noexcept {
        return io_.set_linger(duration);
    }

    [[nodiscard]]
    auto linger() const noexcept {
        return io_.linger();
    }

    // Converts the socket into TcpListener
    [[nodiscard]]
    auto listen(int n) -> Result<TcpListener> {
        if (auto ret = io_.listen(n); !ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        return TcpListener{std::move(io_)};
    }

    // Converts the socket into TcpStream
    [[nodiscard]]
    auto connect(const SocketAddr &address) -> async::Task<Result<TcpStream>> {
        if (auto ret = co_await io_.connect(address); !ret) [[unlikely]] {
            co_return std::unexpected{ret.error()};
        }
        co_return TcpStream{std::move(io_)};
    }

public:
    [[nodiscard]]
    static auto v4() -> Result<TcpSocket> {
        auto io = Socket::build(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (io) [[likely]] {
            return TcpSocket(std::move(io.value()));
        } else {
            return std::unexpected{io.error()};
        }
    }

    [[nodiscard]]
    static auto v6() -> Result<TcpSocket> {
        auto io = Socket::build(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
        if (io) [[likely]] {
            return TcpSocket(std::move(io.value()));
        } else {
            return std::unexpected{io.error()};
        }
    }

private:
    Socket io_;
};

} // namespace zedio::net
