#pragma once

#include "zedio/async/io.hpp"
#include "zedio/net/addr.hpp"
#include "zedio/net/socket.hpp"

// C++
namespace zedio::net {

class UnixStream : public detail::BaseStream<UnixStream, UnixSocketAddr> {
    friend class BaseStream;

    template <class Listener, class Stream, class Addr>
    friend class detail::BaseListener;

    template <class Listener, class Stream, class Addr>
    friend class detail::BaseSocket;

private:
    explicit UnixStream(IO &&io)
        : BaseStream{std::move(io)} {
        LOG_TRACE("Build a UnixStream{{fd: {}}}", io_.fd());
    }

public:
    UnixStream(UnixStream &&other) noexcept = default;

    auto operator=(UnixStream &&other) -> UnixStream & = default;
};

} // namespace zedio::net
