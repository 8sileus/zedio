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

public:
    [[nodiscard]]
    static auto unbound() -> Result<UdpDatagram> {
        int fd = ::socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK, 0);
        if (fd < 0) [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        } else {
            return UdpDatagram{fd};
        }
    }
};

} // namespace zedio::socket::unix_domain
