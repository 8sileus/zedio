#pragma once

#include "zedio/common/debug.hpp"
#include "zedio/common/util/noncopyable.hpp"
#include "zedio/net/accepter.hpp"
#include "zedio/net/tcp/stream.hpp"
// C
#include <cstring>
// C++
#include <expected>
#include <vector>

namespace zedio::net {

class TcpListener : util::Noncopyable {
    friend class TcpSocket;

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
        return detail::Accepter<TcpStream, SocketAddr>{io_.fd()};
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
