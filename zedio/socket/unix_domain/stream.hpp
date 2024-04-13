#pragma once

#include "zedio/socket/stream.hpp"
#include "zedio/socket/unix_domain/addr.hpp"

// C++
namespace zedio::socket::unix_domain {

class TcpStream : public detail::BaseStream<TcpStream, SocketAddr> {
public:
    explicit TcpStream(detail::Socket &&inner)
        : BaseStream{std::move(inner)} {}
};

} // namespace zedio::socket::unix_domain
