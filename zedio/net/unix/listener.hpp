#pragma once

#include "zedio/net/unix/stream.hpp"

namespace zedio::net {

class UnixListener {
    friend class UnixSocket;

private:
    UnixListener(Socket &&io)
        : io_{std::move(io)} {}

public:
    UnixListener(UnixListener &&other) noexcept
        : io_{std::move(other.io_)} {}

    [[nodiscard]]
    auto fd() const noexcept -> int {
        return io_.fd();
    }

    [[nodiscard]]
    auto accept() -> async::Task<Result<std::pair<UnixStream, UnixSocketAddr>>> {
        sockaddr_un addr{};
        socklen_t   length{};
        auto        ret = co_await async::detail::AcceptAwaiter<async::detail::OPFlag::Exclusive>(
            io_.fd(), reinterpret_cast<sockaddr *>(&addr), &length);
        if (!ret) [[unlikely]] {
            co_return std::unexpected{ret.error()};
        }
        co_return std::make_pair(UnixStream{Socket::from_fd(ret.value())},
                                 UnixSocketAddr{reinterpret_cast<sockaddr *>(&addr), length});
    }

public:
    [[nodiscard]]
    static auto bind(const UnixSocketAddr &address) -> Result<UnixListener> {
        auto io = Socket::build(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (!io) [[unlikely]] {
            return std::unexpected{io.error()};
        }
        if (auto ret = io.value().bind(address); !ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        if (auto ret = io.value().listen(SOMAXCONN); !ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        return UnixListener{std::move(io.value())};
    }

private:
    Socket io_;
};

} // namespace zedio::net
