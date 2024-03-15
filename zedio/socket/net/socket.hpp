#pragma once

#include "zedio/socket/net/datagram.hpp"
#include "zedio/socket/net/listener.hpp"
#include "zedio/socket/net/stream.hpp"
#include "zedio/socket/socket.hpp"

namespace zedio::socket::net {

class TcpSocket : public detail::BaseSocket<TcpListener, TcpStream, UdpDatagram, SocketAddr>,
                  public detail::ImplKeepalive<TcpSocket>,
                  public detail::ImplSendBufSize<TcpSocket>,
                  public detail::ImplRecvBufSize<TcpSocket>,
                  public detail::ImplReuseAddr<TcpSocket>,
                  public detail::ImplReusePort<TcpSocket>,
                  public detail::ImplLinger<TcpSocket>,
                  public detail::ImplNodelay<TcpSocket> {
public:
    explicit TcpSocket(const int fd)
        : BaseSocket{fd} {}

public:
    [[nodiscard]]
    static auto v4() -> Result<TcpSocket> {
        auto fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
        if (fd >= 0) [[likely]] {
            return TcpSocket{fd};
        } else {
            return std::unexpected{make_sys_error(errno)};
        }
    }

    [[nodiscard]]
    static auto v6() -> Result<TcpSocket> {
        auto fd = ::socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
        if (fd >= 0) [[likely]] {
            return TcpSocket{fd};
        } else {
            return std::unexpected{make_sys_error(errno)};
        }
    }
};

} // namespace zedio::socket::net
