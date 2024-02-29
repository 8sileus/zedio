#pragma once

#include "zedio/net/addr.hpp"
#include "zedio/net/datagram.hpp"

namespace zedio::net {

class UdpDatagram : public detail::BaseDatagram<UdpDatagram, SocketAddr> {
    friend class BaseDatagram;

private:
    UdpDatagram(detail::SocketIO &&io)
        : BaseDatagram{std::move(io)} {}

public:
    [[nodiscard]]
    auto set_broadcast(bool on) noexcept {
        return io_.set_broadcast(on);
    }

    [[nodiscard]]
    auto broadcast() const noexcept {
        return io_.broadcast();
    }

    [[nodiscard]]
    auto set_ttl(uint32_t ttl) noexcept {
        return io_.set_ttl(ttl);
    }

    [[nodiscard]]
    auto ttl() const noexcept {
        return io_.ttl();
    }

public:
    [[nodiscard]]
    static auto v4() -> Result<UdpDatagram> {
        auto io = detail::SocketIO::build_socket(AF_INET, SOCK_DGRAM, 0);
        if (!io) [[unlikely]] {
            return std::unexpected{io.error()};
        }
        return UdpDatagram{std::move(io.value())};
    }

    [[nodiscard]]
    static auto v6() -> Result<UdpDatagram> {
        auto io = detail::SocketIO::build_socket(AF_INET6, SOCK_DGRAM, 0);
        if (!io) [[unlikely]] {
            return std::unexpected{io.error()};
        }
        return UdpDatagram{std::move(io.value())};
    }
};

} // namespace zedio::net
