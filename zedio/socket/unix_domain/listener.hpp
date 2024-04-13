#pragma once

#include "zedio/socket/listener.hpp"
#include "zedio/socket/unix_domain/stream.hpp"

// Linux
#include <unistd.h>

namespace zedio::socket::unix_domain {

class TcpListener : public detail::BaseListener<TcpListener, TcpStream, SocketAddr> {
public:
    TcpListener(detail::Socket &&inner)
        : BaseListener{std::move(inner)} {}

    ~TcpListener() {
        if (fd() >= 0) {
            if (auto ret = this->local_addr(); ret) [[likely]] {
                if (::unlink(ret.value().pathname().data()) != 0) [[unlikely]] {
                    LOG_ERROR("{}", strerror(errno));
                }
            } else {
                LOG_ERROR("{}", ret.error().message());
            }
        }
    }

    TcpListener(TcpListener &&other) noexcept = default;
    auto operator=(TcpListener &&other) noexcept -> TcpListener & = default;
};

} // namespace zedio::socket::unix_domain
