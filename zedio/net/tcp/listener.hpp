#pragma once

#include "zedio/net/listener.hpp"
#include "zedio/net/tcp/stream.hpp"

namespace zedio::net {

class TcpListener : public detail::BaseListener<TcpListener, TcpStream, SocketAddr> {
    friend class detail::SocketIO;

    explicit TcpListener(detail::SocketIO &&io)
        : BaseListener{std::move(io)} {
        LOG_TRACE("Build a TcpListener {{fd: {}}}", io_.fd());
    }

public:
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
