#pragma once

#include "zedio/socket/listener.hpp"
#include "zedio/socket/net/stream.hpp"

namespace zedio::socket::net {

class TcpListener : public detail::BaseListener<TcpListener, TcpStream, SocketAddr>,
                    public detail::ImplTTL<TcpListener> {
public:
    explicit TcpListener(const int fd)
        : BaseListener{fd} {
        LOG_TRACE("Build a TcpListener {{fd: {}}}", fd);
    }
};

} // namespace zedio::socket::net
