#pragma once

#include "zedio/socket/datagram.hpp"
#include "zedio/socket/unix_domain/addr.hpp"

namespace zedio::socket::unix_domain {

class UdpDatagram : public detail::BaseDatagram<UdpDatagram, SocketAddr>,
                    public detail::ImplPasscred<UdpDatagram>,
                    public detail::ImplMark<UdpDatagram> {
public:
    UdpDatagram(const int fd)
        : BaseDatagram{fd} {}
};

} // namespace zedio::socket::unix_domain
