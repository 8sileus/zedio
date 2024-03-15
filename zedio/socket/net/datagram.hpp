#pragma once

#include "zedio/socket/datagram.hpp"
#include "zedio/socket/net/addr.hpp"

namespace zedio::socket::net {

class UdpDatagram : public detail::BaseDatagram<UdpDatagram, SocketAddr>,
                    public detail::ImplBoradcast<UdpDatagram>,
                    public detail::ImplTTL<UdpDatagram> {
public:
    explicit UdpDatagram(const int fd)
        : BaseDatagram{fd} {}

public:
    [[nodiscard]]
    static auto v4() -> Result<UdpDatagram> {
        auto fd = ::socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP);
        if (fd >= 0) [[likely]] {
            return UdpDatagram{fd};
        } else {
            return std::unexpected{make_sys_error(errno)};
        }
    }

    [[nodiscard]]
    static auto v6() -> Result<UdpDatagram> {
        auto fd = ::socket(AF_INET6, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP);
        if (fd >= 0) [[likely]] {
            return UdpDatagram{fd};
        } else {
            return std::unexpected{make_sys_error(errno)};
        }
    }
};

} // namespace zedio::socket::net
