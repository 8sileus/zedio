#pragma once

#include "zedio/net/addr.hpp"

// C++
namespace zedio::net {

class UnixStream : public detail::BaseStream<UnixStream, UnixSocketAddr> {
public:
    explicit UnixStream(IO &&io)
        : BaseStream{std::move(io)} {
        LOG_TRACE("Build a UnixStream{{fd: {}}}", io_.fd());
    }

    UnixStream(UnixStream &&other) noexcept = default;

    auto operator=(UnixStream &&other) -> UnixStream & = default;
};

} // namespace zedio::net
