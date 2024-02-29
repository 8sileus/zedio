#pragma once

#include "zedio/net/addr.hpp"
#include "zedio/net/stream.hpp"

namespace zedio::net {

class TcpStream : public detail::BaseStream<TcpStream, SocketAddr> {
    friend class detail::SocketIO;

    explicit TcpStream(detail::SocketIO &&io)
        : BaseStream{std::move(io)} {
        LOG_TRACE("Build a TcpStream{{fd: {}}}", io_.fd());
    }

public:
    [[nodiscard]]
    auto set_nodelay(bool need_delay) noexcept {
        return io_.set_nodelay(need_delay);
    }

    [[nodiscard]]
    auto nodelay() const noexcept {
        return io_.nodelay();
    }

    [[nodiscard]]
    auto set_linger(std::optional<std::chrono::seconds> duration) noexcept {
        return io_.set_linger(duration);
    }

    [[nodiscard]]
    auto linger() const noexcept {
        return io_.linger();
    }

    [[nodiscard]]
    auto set_ttl(uint32_t ttl) noexcept {
        return io_.set_ttl(ttl);
    }

    [[nodiscard]]
    auto ttl() const noexcept {
        return io_.ttl();
    }
};

} // namespace zedio::net
