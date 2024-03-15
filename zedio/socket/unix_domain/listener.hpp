#pragma once

#include "zedio/socket/listener.hpp"
#include "zedio/socket/unix_domain/stream.hpp"

// Linux
#include <unistd.h>

namespace zedio::socket::unix_domain {

class TcpListener : public detail::BaseListener<TcpListener, TcpStream, SocketAddr> {
public:
    TcpListener(const int fd)
        : BaseListener{fd} {
        LOG_TRACE("Build a TcpListener {{fd: {}}}", fd);
    }

    TcpListener(TcpListener &&) = default;
    auto operator=(TcpListener &&) -> TcpListener & = default;

    ~TcpListener() {
        if (fd_ >= 0) {
            if (auto ret = this->local_addr(); ret) [[likely]] {
                if (::unlink(ret.value().pathname().data()) != 0) [[unlikely]] {
                    LOG_ERROR("{}", strerror(errno));
                }
            } else {
                LOG_ERROR("{}", ret.error().message());
            }
        }
    }
};

} // namespace zedio::socket::unix_domain
