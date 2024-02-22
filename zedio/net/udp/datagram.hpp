#pragma once

#include "zedio/net/addr.hpp"
#include "zedio/net/datagram.hpp"

namespace zedio::net {

class UdpDatagram : public detail::BaseDatagram<UdpDatagram, SocketAddr> {
    friend class BaseDatagram;

private:
    UdpDatagram(IO &&io)
        : BaseDatagram{std::move(io)} {}

public:
    [[nodiscard]]
    auto set_broadcast(bool on) const noexcept {
        return io_.set_broadcast(on);
    }

    [[nodiscard]]
    auto broadcast() const noexcept {
        return io_.broadcast();
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
    static auto unbound(bool is_ipv6 = false) -> Result<UdpDatagram> {
        auto io = IO::socket(is_ipv6 ? AF_INET6 : AF_INET, SOCK_DGRAM, 0);
        if (!io) [[unlikely]] {
            return std::unexpected{io.error()};
        }
        return UdpDatagram{std::move(io.value())};
    }
};

} // namespace zedio::net
