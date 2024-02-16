#pragma once

#include "zedio/common/error.hpp"
#include "zedio/net/socket.hpp"
#include "zedio/net/unix/addr.hpp"
#include "zedio/net/unix/listener.hpp"
// Linux
#include <sys/un.h>

namespace zedio::net {

class UnixSocket {
private:
    explicit UnixSocket(Socket &&io)
        : io_{std::move(io)} {}

public:
    UnixSocket(UnixSocket &&other) noexcept
        : io_{std::move(other.io_)} {}

    [[nodiscard]]
    auto bind(const UnixSocketAddr &addr) const noexcept {
        return io_.bind(addr);
    }

    [[nodiscard]]
    auto fd() const noexcept {
        return io_.fd();
    }

    [[nodiscard]]
    auto local_addr() const noexcept {
        return io_.local_addr<UnixSocketAddr>();
    }

    [[nodiscard]]
    auto listen(int n) -> Result<UnixListener> {
        if (auto ret = io_.listen(n); !ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        return UnixListener{std::move(io_)};
    }

    [[nodiscard]]
    auto connect(const UnixSocketAddr &address) -> async::Task<Result<UnixStream>> {
        if (auto ret = co_await io_.connect(address); !ret) [[unlikely]] {
            co_return std::unexpected{ret.error()};
        }
        co_return UnixStream{std::move(io_)};
    }

public:
    [[nodiscard]]
    auto stream() -> Result<UnixSocket> {
        if (auto io = Socket::build(AF_UNIX, SOCK_STREAM, 0); io) {
            return UnixSocket{std::move(io.value())};
        } else {
            return std::unexpected{io.error()};
        }
    }

    [[nodiscard]]
    auto datagram() -> Result<UnixSocket> {
        if (auto io = Socket::build(AF_UNIX, SOCK_DGRAM, 0); io) {
            return UnixSocket{std::move(io.value())};
        } else {
            return std::unexpected{io.error()};
        }
    }

private:
    Socket io_;
};

} // namespace zedio::net
