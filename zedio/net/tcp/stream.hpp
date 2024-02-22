#pragma once

#include "zedio/net/addr.hpp"
#include "zedio/net/socket.hpp"
#include "zedio/net/stream.hpp"
// C++
#include <chrono>
#include <optional>
#include <span>
// Linux
#include <sys/socket.h>

namespace zedio::net {

class TcpStream : public detail::BaseStream<TcpStream, SocketAddr> {
    friend class BaseStream;

    template <class Listener, class Stream, class Addr>
    friend class detail::BaseListener;

    template <class Listener, class Stream, class Addr>
    friend class detail::BaseSocket;

private:
    explicit TcpStream(IO &&io)
        : BaseStream{std::move(io)} {
        LOG_TRACE("Build a TcpStream{{fd: {}}}", io_.fd());
    }

public:
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
