#pragma once

#include "zedio/net/socket.hpp"
#include "zedio/net/unix/listener.hpp"
#include "zedio/net/unix/stream.hpp"
// Linux
#include <sys/un.h>

namespace zedio::net {

class UnixSocket : public detail::BaseSocket<UnixListener, UnixStream, UnixSocketAddr> {
private:
    explicit UnixSocket(detail::SocketIO &&io)
        : BaseSocket{std::move(io)} {}

public:
    UnixSocket(UnixSocket &&other) noexcept = default;

public:
    [[nodiscard]]
    auto stream() -> Result<UnixSocket> {
        if (auto io = detail::SocketIO::build_socket(AF_UNIX, SOCK_STREAM, 0); io) {
            return UnixSocket{std::move(io.value())};
        } else {
            return std::unexpected{io.error()};
        }
    }

    [[nodiscard]]
    auto datagram() -> Result<UnixSocket> {
        if (auto io = detail::SocketIO::build_socket(AF_UNIX, SOCK_DGRAM, 0); io) {
            return UnixSocket{std::move(io.value())};
        } else {
            return std::unexpected{io.error()};
        }
    }
};

} // namespace zedio::net
