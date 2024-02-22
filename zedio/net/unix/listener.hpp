#pragma once

#include "zedio/net/listener.hpp"
#include "zedio/net/unix/stream.hpp"

// Linux
#include <unistd.h>

namespace zedio::net {

class UnixListener : public detail::BaseListener<UnixListener, UnixStream, UnixSocketAddr> {
    friend class UnixSocket;
    friend class BaseListener;

private:
    UnixListener(IO &&io)
        : BaseListener{std::move(io)} {
        LOG_TRACE("Build a UnixListener {{fd: {}}}", io_.fd());
    }

public:
    UnixListener(UnixListener &&other) noexcept = default;

    ~UnixListener() {
        if (io_.fd() >= 0) {
            // TODO register unlink
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
