#pragma once

#include "zedio/common/error.hpp"
#include "zedio/net/socket.hpp"
#include "zedio/net/tcp/listener.hpp"
#include "zedio/net/tcp/stream.hpp"
// C++
#include <expected>

namespace zedio::net {

class TcpSocket : public detail::BaseSocket<TcpListener, TcpStream, SocketAddr> {
private:
    explicit TcpSocket(IO &&io)
        : BaseSocket{std::move(io)} {}

public:
    TcpSocket(TcpSocket &&other) = default;

    [[nodiscard]]
    auto set_keepalive(bool on) const noexcept {
        return io_.set_keepalive(on);
    }

    [[nodiscard]]
    auto keepalive() const noexcept {
        return io_.keepalive();
    }

    [[nodiscard]]
    auto set_send_buffer_size(std::size_t size) const noexcept {
        return io_.set_send_buffer_size(size);
    }

    [[nodiscard]]
    auto send_buffer_size() const noexcept {
        return io_.send_buffer_size();
    }

    [[nodiscard]]
    auto set_recv_buffer_size(std::size_t size) const noexcept {
        return io_.set_recv_buffer_size(size);
    }

    [[nodiscard]]
    auto recv_buffer_size() const noexcept {
        return io_.recv_buffer_size();
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

public:
    [[nodiscard]]
    static auto v4() -> Result<TcpSocket> {
        auto io = IO::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (io) [[likely]] {
            return TcpSocket(std::move(io.value()));
        } else {
            return std::unexpected{io.error()};
        }
    }

    [[nodiscard]]
    static auto v6() -> Result<TcpSocket> {
        auto io = IO::socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
        if (io) [[likely]] {
            return TcpSocket(std::move(io.value()));
        } else {
            return std::unexpected{io.error()};
        }
    }
};

} // namespace zedio::net
