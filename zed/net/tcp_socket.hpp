#pragma once

#include "zed/common/error.hpp"
#include "zed/net/tcp_listener.hpp"
// C++
#include <expected>

namespace zed::net {

class TcpSocket : util::Noncopyable {
    explicit TcpSocket(Socket &&sock)
        : socket_{std::move(sock)} {}

public:
    TcpSocket(TcpSocket &&other)
        : socket_{std::move(other.socket_)} {}

    [[nodiscard]]
    auto bind(const SocketAddr &address) const noexcept {
        return socket_.bind(address);
    }

    [[nodiscard]]
    auto fd() const noexcept {
        return socket_.fd();
    }

    [[nodiscard]]
    auto set_reuseaddr(bool on)const noexcept  {
        return socket_.set_reuseaddr(on);
    }

    [[nodiscard]]
    auto reuseaddr() const noexcept {
        return socket_.reuseaddr();
    }

    [[nodiscard]]
    auto set_reuseport(bool on) const noexcept {
        return socket_.set_reuseport(on);
    }

    [[nodiscard]]
    auto reuseport() const noexcept {
        return socket_.reuseport();
    }

    [[nodiscard]]
    auto set_nonblocking(bool on) const noexcept {
        return socket_.set_nonblocking(on);
    }

    [[nodiscard]]
    auto nonblocking() const noexcept {
        return socket_.nonblocking();
    }

    [[nodiscard]]
    auto set_nodelay(bool on) const noexcept {
        return socket_.set_nodelay(on);
    }

    [[nodiscard]]
    auto nodelay() const noexcept {
        return socket_.nodelay();
    }

    [[nodiscard]]
    auto set_linger(std::optional<std::chrono::seconds> duration) const noexcept {
        return socket_.set_linger(duration);
    }

    [[nodiscard]]
    auto linger() const noexcept {
        return socket_.linger();
    }

    // Converts the socket into TcpListener
    [[nodiscard]]
    auto listen(int n) -> Result<TcpListener> {
        if (auto ret = socket_.listen(n); !ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        return TcpListener{std::move(socket_)};
    }

    // Converts the socket into TcpStream
    [[nodiscard]]
    auto connect(const SocketAddr &address)
        -> async::Task<std::expected<TcpStream, std::error_code>> {
        if (auto ret = co_await socket_.connect(address); !ret) [[unlikely]] {
            co_return std::unexpected{ret.error()};
        }
        co_return TcpStream{std::move(socket_)};
    }

public:
    [[nodiscard]]
    static auto v4() -> Result<TcpSocket> {
        auto sock = Socket::build(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
        if (sock) [[likely]] {
            return TcpSocket(std::move(sock.value()));
        } else {
            return std::unexpected{sock.error()};
        }
    }

    [[nodiscard]]
    static auto v6() -> Result<TcpSocket> {
        auto sock = Socket::build(AF_INET6, SOCK_STREAM |SOCK_NONBLOCK, IPPROTO_TCP);
        if (sock) [[likely]] {
            return TcpSocket(std::move(sock.value()));
        } else {
            return std::unexpected{sock.error()};
        }
    }

private:
    Socket socket_;
};

} // namespace zed::net
