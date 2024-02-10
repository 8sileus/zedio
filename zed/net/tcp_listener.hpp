#pragma once

#include "zed/common/debug.hpp"
#include "zed/common/util/noncopyable.hpp"
#include "zed/net/socket_addr.hpp"
#include "zed/net/tcp_stream.hpp"
// C
#include <cstring>
// C++
#include <expected>
#include <vector>

namespace zed::net {

namespace detail {
    struct [[REMEMBER_CO_AWAIT]] AcceptStream
        : public async::detail::AcceptAwaiter<async::detail::OPFlag::Registered> {
        AcceptStream(int fd)
            : AcceptAwaiter{fd, reinterpret_cast<sockaddr *>(&addr), &addrlen} {}

        auto await_resume() const noexcept -> Result<std::pair<TcpStream, SocketAddr>> {
            auto result = AcceptAwaiter::await_resume();
            if (result.has_value()) [[likely]] {
                return std::make_pair(
                    TcpStream{Socket::from_fd(result.value())},
                    SocketAddr{reinterpret_cast<const sockaddr *>(&addr), addrlen});
            }
            return std::unexpected{result.error()};
        }
        sockaddr_in6 addr;
        socklen_t    addrlen;
    };
} // namespace detail

class TcpSocket;

class TcpListener : util::Noncopyable {
    friend class TcpSocket;

private:
    explicit TcpListener(Socket &&sock)
        : socket_{std::move(sock)}
        , idx_{async::detail::t_poller->register_file(socket_.fd()).value()} {
        LOG_TRACE("Build a TcpListener {{fd: {}}}", socket_.fd());
    }

public:
    ~TcpListener() {
        if (this->socket_.fd() >= 0) [[likely]] {
            async::detail::t_poller->unregister_file(idx_);
        }
    }

    TcpListener(TcpListener &&other)
        : socket_{std::move(other.socket_)}
        , idx_{other.idx_} {
        other.idx_ = -1;
    }

    auto operator=(TcpListener &&other) -> TcpListener & {
        socket_ = std::move(other.socket_);
        idx_ = other.idx_;
        other.idx_ = -1;
        return *this;
    }

    [[nodiscard]]
    auto accept() const noexcept {
        return detail::AcceptStream{idx_};
    }

    [[nodiscard]]
    auto local_addr() const noexcept {
        return socket_.local_addr();
    }

    [[nodiscard]]
    auto fd() const noexcept {
        return socket_.fd();
    }

    [[nodiscard]]
    auto set_ttl(uint32_t ttl) const noexcept {
        return socket_.set_ttl(ttl);
    }

    [[nodiscard]]
    auto ttl() const noexcept {
        return socket_.ttl();
    }

public:
    [[nodiscard]]
    static auto bind(const SocketAddr &address) -> std::expected<TcpListener, std::error_code> {
        auto sock = Socket::build(address.family(), SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
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

private:
    Socket socket_;
    int    idx_{-1};
};

} // namespace zed::net
