#pragma once

#include "zedio/common/debug.hpp"
#include "zedio/common/util/noncopyable.hpp"
#include "zedio/net/tcp/stream.hpp"
// C
#include <cstring>
// C++
#include <expected>
#include <vector>

namespace zedio::net {

class TcpListener : util::Noncopyable {
    friend class TcpSocket;

    class TcpAccepter : public async::detail::AcceptAwaiter<async::detail::OPFlag::Distributive> {
    public:
        TcpAccepter(int fd)
            : AcceptAwaiter(fd, reinterpret_cast<struct sockaddr *>(&addr_), &addrlen_) {}

        auto await_resume() const noexcept -> Result<std::pair<TcpStream, SocketAddr>> {
            auto ret = AcceptAwaiter::BaseIOAwaiter::await_resume();
            if (!ret) [[unlikely]] {
                return std::unexpected{ret.error()};
            }
            return std::make_pair(TcpStream{Socket::from_fd(ret.value())},
                                  SocketAddr{reinterpret_cast<const sockaddr *>(&addr_), addrlen_});
        }

    private:
        struct sockaddr_in6 addr_ {};
        socklen_t           addrlen_{0};
    };

private:
    explicit TcpListener(Socket &&sock)
        : io_{std::move(sock)} {
        LOG_TRACE("Build a TcpListener {{fd: {}}}", io_.fd());
    }

public:
    TcpListener(TcpListener &&other)
        : io_{std::move(other.io_)} {}

    [[nodiscard]]
    auto accept() const noexcept {
        return TcpAccepter{io_.fd()};
    }

    [[nodiscard]]
    auto local_addr() const noexcept {
        return io_.local_addr<SocketAddr>();
    }

    [[nodiscard]]
    auto fd() const noexcept {
        return io_.fd();
    }

    [[nodiscard]]
    auto set_ttl(uint32_t ttl) const noexcept {
        return io_.set_ttl(ttl);
    }

    [[nodiscard]]
    auto ttl() const noexcept {
        return io_.ttl();
    }

public:
    [[nodiscard]]
    static auto bind(const SocketAddr &address) -> Result<TcpListener> {
        auto sock = Socket::build(address.family(), SOCK_STREAM, IPPROTO_TCP);
        if (!sock) [[unlikely]] {
            return std::unexpected{sock.error()};
        }
        if (auto ret = sock.value().bind(address); !ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        if (auto ret = sock.value().listen(SOMAXCONN); !ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        return TcpListener{std::move(sock.value())};
    }

    [[nodiscard]]
    static auto bind(const std::span<SocketAddr> &addresses) -> Result<TcpListener> {
        for (const auto &address : addresses) {
            if (auto ret = bind(address); ret) [[likely]] {
                return ret;
            } else {
                LOG_ERROR("Bind {} failed, error: {}", address.to_string(), ret.error().message());
            }
        }
        return std::unexpected{make_zedio_error(Error::InvalidInput)};
    }

private:
    Socket io_;
};

} // namespace zedio::net
