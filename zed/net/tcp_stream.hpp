#pragma once

#include "zed/net/socket.hpp"
// C++
#include <chrono>
#include <optional>
#include <span>
// Linux
#include <sys/socket.h>

namespace zed::net {

class TcpSocket;

namespace detail {
    class AcceptStream;
} // namespace detail

class TcpStream : util::Noncopyable {
    friend class TcpSocket;
    friend class detail::AcceptStream;

private:
    explicit TcpStream(Socket &&sock)
        : socket_{std::move(sock)} {
        LOG_TRACE("Build a TcpStream{{fd: {}}}", socket_.fd());
    }

public:
    TcpStream(TcpStream &&other)
        : socket_(std::move(other.socket_)) {}

    auto operator=(TcpStream &&other) -> TcpStream & {
        socket_ = std::move(other.socket_);
        return *this;
    }

    [[nodiscard]]
    auto shutdown(SHUTDOWN_OPTION how) const noexcept {
        return socket_.shutdown(how);
    }

    [[nodiscard]]
    auto read(std::span<char> buf) const noexcept {
        return socket_.read(buf);
    }

    template <typename... Ts>
    [[nodiscard]]
    auto read_vectored(Ts &...bufs) const noexcept {
        return socket_.read_vectored(bufs...);
    }

    [[nodiscard]]
    auto write(std::span<const char> buf) const noexcept {
        return socket_.write(buf);
    }

    template <typename... Ts>
    [[nodiscard]]
    auto write_vectored(Ts &...bufs) const noexcept {
        return socket_.write_vectored(bufs...);
    }

    [[nodiscard]]
    auto try_read(std::span<char> buf) const noexcept {
        return socket_.try_read(buf);
    }

    template <typename... Ts>
    [[nodiscard]]
    auto try_read_vectored(Ts &...bufs) const noexcept {
        return socket_.try_read_vectored(bufs...);
    }

    [[nodiscard]]
    auto try_write(std::span<const char> buf) const noexcept {
        return socket_.try_write(buf);
    }

    template <typename... Ts>
    [[nodiscard]]
    auto try_write_vectored(Ts &...bufs) const noexcept {
        return socket_.try_write_vectored(bufs...);
    }

    [[nodiscard]]
    auto local_addr() const noexcept {
        return socket_.local_addr();
    }

    [[nodiscard]]
    auto peer_addr() const noexcept {
        return socket_.peer_addr();
    }

    [[nodiscard]]
    auto fd() const noexcept {
        return socket_.fd();
    }

    [[nodiscard]]
    auto set_nodelay(bool need_delay) const noexcept {
        return socket_.set_nodelay(need_delay);
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

    [[nodiscard]]
    auto set_ttl(uint32_t ttl) const noexcept {
        return socket_.set_ttl(ttl);
    }

    [[nodiscard]]
    auto ttl() const noexcept {
        return socket_.ttl();
    }

public:
    [[nodiscard]]
    static auto connect(const SocketAddr &address)
        -> async::Task<std::expected<TcpStream, std::error_code>> {
        auto sock = Socket::build(address.family(), SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
        if (!sock) [[unlikely]] {
            co_return std::unexpected{sock.error()};
        }
        if (auto ret = co_await sock.value().connect(address); !ret) [[unlikely]] {
            co_return std::unexpected{ret.error()};
        }
        co_return TcpStream{std::move(sock.value())};
    };

private:
    Socket socket_;
};

} // namespace zed::net
