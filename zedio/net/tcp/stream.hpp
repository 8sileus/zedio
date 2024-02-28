#pragma once

#include "zedio/net/addr.hpp"
#include "zedio/net/stream.hpp"
// C++
#include <chrono>
#include <optional>

namespace zedio::net {

class TcpStream : public detail::BaseStream<TcpStream, SocketAddr> {
public:
    explicit TcpStream(IO &&io)
        : BaseStream{std::move(io)} {
        LOG_TRACE("Build a TcpStream{{fd: {}}}", io_.fd());
    }

    TcpStream(TcpStream &&other) = default;
    auto operator=(TcpStream &&other) -> TcpStream & = default;

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
};

} // namespace zedio::net
