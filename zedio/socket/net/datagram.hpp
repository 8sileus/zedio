#pragma once

#include "zedio/socket/datagram.hpp"
#include "zedio/socket/impl/impl_sockopt.hpp"
#include "zedio/socket/net/addr.hpp"

namespace zedio::socket::net {

class UdpDatagram : public detail::BaseDatagram<UdpDatagram, SocketAddr>,
                    public detail::ImplBoradcast<UdpDatagram>,
                    public detail::ImplTTL<UdpDatagram> {

public:
    explicit UdpDatagram(detail::Socket &&inner)
        : BaseDatagram{std::move(inner)} {}

public:
    [[nodiscard]]
    static auto unbound(bool is_ipv6 = false) -> Result<UdpDatagram> {
        return detail::Socket::create<UdpDatagram>(is_ipv6 ? AF_INET6 : AF_INET,
                                                   SOCK_DGRAM,
                                                   IPPROTO_UDP);
    }
};

} // namespace zedio::socket::net
