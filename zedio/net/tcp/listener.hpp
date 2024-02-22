#pragma once

#include "zedio/common/debug.hpp"
#include "zedio/net/addr.hpp"
#include "zedio/net/listener.hpp"
#include "zedio/net/tcp/stream.hpp"
// C
#include <cstring>

namespace zedio::net {

class TcpListener : public detail::BaseListener<TcpListener, TcpStream, SocketAddr> {
    friend class TcpSocket;
    friend class BaseListener;

private:
    explicit TcpListener(IO &&io)
        : BaseListener{std::move(io)} {
        LOG_TRACE("Build a TcpListener {{fd: {}}}", io_.fd());
    }

public:
    TcpListener(TcpListener &&other) noexcept = default;

    auto operator=(TcpListener &&other) noexcept -> TcpListener & = default;

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
