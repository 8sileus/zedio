#pragma once

#include "zedio/socket/stream.hpp"
#include "zedio/socket/unix_domain/addr.hpp"

// C++
namespace zedio::socket::unix_domain {

class TcpStream : public detail::BaseStream<TcpStream, SocketAddr> {
public:
    explicit TcpStream(const int fd)
        : BaseStream{fd} {
        LOG_TRACE("Build a UnixStream{{fd: {}}}", fd);
    }
};

} // namespace zedio::socket::unix_domain
