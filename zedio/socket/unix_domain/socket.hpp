#pragma once

#include "zedio/socket/socket.hpp"
#include "zedio/socket/unix_domain/datagram.hpp"
#include "zedio/socket/unix_domain/listener.hpp"
#include "zedio/socket/unix_domain/stream.hpp"
// Linux
#include <sys/un.h>

namespace zedio::socket::unix_domain {

class Socket : public detail::BaseSocket<TcpListener, TcpStream, UdpDatagram, SocketAddr> {
public:
    explicit Socket(const int fd)
        : BaseSocket{fd} {}

public:
    [[nodiscard]]
    auto stream() -> Result<Socket> {
        int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd >= 0) {
            return Socket{fd};
        } else {
            return std::unexpected{make_sys_error(errno)};
        }
    }

    [[nodiscard]]
    auto datagram() -> Result<Socket> {
        int fd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
        if (fd >= 0) {
            return Socket{fd};
        } else {
            return std::unexpected{make_sys_error(errno)};
        }
    }

private:
};

} // namespace zedio::socket::unix_domain
