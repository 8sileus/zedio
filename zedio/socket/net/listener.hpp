#pragma once

#include "zedio/socket/listener.hpp"
#include "zedio/socket/net/stream.hpp"

namespace zedio::socket::net {

class TcpListener : public detail::BaseListener<TcpListener, TcpStream, SocketAddr>,
                    public detail::ImplTTL<TcpListener> {
public:
    explicit TcpListener(detail::Socket &&inner)
        : BaseListener{std::move(inner)} {}
};

} // namespace zedio::socket::net
