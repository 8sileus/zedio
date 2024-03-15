#pragma once

#include "zedio/socket/net/addr.hpp"
#include "zedio/socket/stream.hpp"

namespace zedio::socket::net {

class TcpStream : public detail::BaseStream<TcpStream, SocketAddr>,
                  public socket::detail::ImplNodelay<TcpStream>,
                  public socket::detail::ImplLinger<TcpStream>,
                  public socket::detail::ImplTTL<TcpStream> {
public:
    explicit TcpStream(const int fd)
        : BaseStream{fd} {
        LOG_TRACE("Build a TcpStream{{fd: {}}}", fd);
    }
};

} // namespace zedio::socket::net
