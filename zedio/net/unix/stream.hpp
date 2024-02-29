#pragma once

#include "zedio/net/addr.hpp"
#include "zedio/net/stream.hpp"

// C++
namespace zedio::net {

class UnixStream : public detail::BaseStream<UnixStream, UnixSocketAddr> {
    friend class detail::SocketIO;

public:
    explicit UnixStream(detail::SocketIO &&io)
        : BaseStream{std::move(io)} {
        LOG_TRACE("Build a UnixStream{{fd: {}}}", io_.fd());
    }
};

} // namespace zedio::net
