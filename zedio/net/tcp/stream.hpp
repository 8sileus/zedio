#pragma once

#include "zedio/net/addr.hpp"
#include "zedio/net/socket.hpp"
// C++
#include <chrono>
#include <optional>
#include <span>
// Linux
#include <sys/socket.h>

namespace zedio::net {

class TcpStream : util::Noncopyable {
    friend class TcpSocket;

    template <class T1, class T2>
    friend class detail::Accepter;

private:
    explicit TcpStream(Socket &&sock)
        : io_{std::move(sock)} {
        LOG_TRACE("Build a TcpStream{{fd: {}}}", io_.fd());
    }

public:
    TcpStream(TcpStream &&other)
        : io_(std::move(other.io_)) {}

    auto operator=(TcpStream &&other) -> TcpStream & {
        io_ = std::move(other.io_);
        return *this;
    }

    [[nodiscard]]
    auto shutdown(SHUTDOWN_OPTION how) const noexcept {
        return io_.shutdown(how);
    }

    [[nodiscard]]
    auto close() noexcept {
        return io_.close();
    }

    [[nodiscard]]
    auto read(std::span<char> buf) const noexcept {
        return io_.read(buf);
    }

    template <typename... Ts>
    [[nodiscard]]
    auto read_vectored(Ts &...bufs) const noexcept {
        return io_.read_vectored(bufs...);
    }

    [[nodiscard]]
    auto write(std::span<const char> buf) const noexcept {
        return io_.send(buf);
    }

    [[nodiscard]]
    auto write_all(std::span<const char> buf) const noexcept {
        return io_.write_all(buf);
    }

    template <typename... Ts>
    [[nodiscard]]
    auto write_vectored(Ts &...bufs) const noexcept {
        return io_.write_vectored(bufs...);
    }

    [[nodiscard]]
    auto local_addr() const noexcept {
        return io_.local_addr<SocketAddr>();
    }

    [[nodiscard]]
    auto peer_addr() const noexcept {
        return io_.peer_addr<SocketAddr>();
    }

    [[nodiscard]]
    auto fd() const noexcept {
        return io_.fd();
    }

    [[nodiscard]]
    auto set_nodelay(bool need_delay) const noexcept {
        return io_.set_nodelay(need_delay);
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

    [[nodiscard]]
    auto set_ttl(uint32_t ttl) const noexcept {
        return io_.set_ttl(ttl);
    }

    [[nodiscard]]
    auto ttl() const noexcept {
        return io_.ttl();
    }

public:
    [[nodiscard]]
    static auto connect(const SocketAddr &address) -> async::Task<Result<TcpStream>> {
        auto sock = Socket::build(address.family(), SOCK_STREAM, IPPROTO_TCP);
        if (!sock) [[unlikely]] {
            co_return std::unexpected{sock.error()};
        }
        if (auto ret = co_await sock.value().connect(address); !ret) [[unlikely]] {
            co_return std::unexpected{ret.error()};
        }
        co_return TcpStream{std::move(sock.value())};
    };

private:
    Socket io_;
};

} // namespace zedio::net
