#pragma once

#include "zedio/socket/datagram.hpp"
#include "zedio/socket/unix_domain/addr.hpp"

namespace zedio::socket::unix_domain {

class UdpDatagram : public detail::BaseDatagram<UdpDatagram, SocketAddr> {
public:
    explicit UdpDatagram(detail::Socket &&inner)
        : BaseDatagram{std::move(inner)} {}

public:
    [[nodiscard]]
    static auto unbound() -> Result<UdpDatagram> {
        return detail::Socket::create<UdpDatagram>(AF_UNIX, SOCK_DGRAM, 0);
    }
};

} // namespace zedio::socket::unix_domain
