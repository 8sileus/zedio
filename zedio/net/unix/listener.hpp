#pragma once

#include "zedio/net/unix/stream.hpp"

// Linux
#include <unistd.h>

namespace zedio::net {

class UnixListener {
    friend class UnixSocket;

private:
    class UnixAccepter : public async::detail::AcceptAwaiter<async::detail::OPFlag::Distributive> {
    public:
        UnixAccepter(int fd)
            : AcceptAwaiter(fd, reinterpret_cast<struct sockaddr *>(&addr_), &addrlen_) {}

        auto await_resume() const noexcept -> Result<std::pair<UnixStream, UnixSocketAddr>> {
            auto ret = AcceptAwaiter::BaseIOAwaiter::await_resume();
            if (!ret) [[unlikely]] {
                return std::unexpected{ret.error()};
            }
            return std::make_pair(
                UnixStream{Socket::from_fd(ret.value())},
                UnixSocketAddr{reinterpret_cast<const sockaddr *>(&addr_), addrlen_});
        }

    private:
        struct sockaddr_un addr_ {};
        socklen_t          addrlen_{0};
    };

private:
    UnixListener(Socket &&io)
        : io_{std::move(io)} {}

public:
    UnixListener(UnixListener &&other) noexcept
        : io_{std::move(other.io_)} {}

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

    [[nodiscard]]
    auto fd() const noexcept -> int {
        return io_.fd();
    }

    [[nodiscard]]
    auto accept() {
        return UnixAccepter{io_.fd()};
    }

    [[nodiscard]]
    auto local_addr() {
        return io_.local_addr<UnixSocketAddr>();
    }

public:
    [[nodiscard]]
    static auto bind(const UnixSocketAddr &address) -> Result<UnixListener> {
        auto io = Socket::build(AF_UNIX, SOCK_STREAM, 0);
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
