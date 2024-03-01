#pragma once

#include "zedio/net/listener.hpp"
#include "zedio/net/unix/stream.hpp"

// Linux
#include <unistd.h>

namespace zedio::net {

class UnixListener : public detail::BaseListener<UnixListener, UnixStream, UnixSocketAddr> {
    friend class detail::SocketIO;

private:
    UnixListener(detail::SocketIO &&io)
        : BaseListener{std::move(io)} {
        LOG_TRACE("Build a UnixListener {{fd: {}}}", io_.fd());
    }

public:
    UnixListener(UnixListener &&other) noexcept = default;

    ~UnixListener() {
        if (io_.fd() >= 0) {
            if (auto ret = io_.local_addr<UnixSocketAddr>(); ret) [[likely]] {
                if (::unlink(ret.value().pathname().data()) != 0) [[unlikely]] {
                    LOG_ERROR("{}", strerror(errno));
                }
            } else {
                LOG_ERROR("{}", ret.error().message());
            }
        }
    }
};

} // namespace zedio::net
