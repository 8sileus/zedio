#pragma once

#include "zedio/socket/net/datagram.hpp"
#include "zedio/socket/net/listener.hpp"
#include "zedio/socket/net/stream.hpp"
#include "zedio/socket/socket.hpp"

namespace zedio::socket::net {

class TcpSocket : public detail::ImplKeepalive<TcpSocket>,
                  public detail::ImplSendBufSize<TcpSocket>,
                  public detail::ImplRecvBufSize<TcpSocket>,
                  public detail::ImplReuseAddr<TcpSocket>,
                  public detail::ImplReusePort<TcpSocket>,
                  public detail::ImplLinger<TcpSocket>,
                  public detail::ImplNodelay<TcpSocket> {
    using Socket = detail::Socket;

public:
    explicit TcpSocket(Socket &&inner)
        : inner_{std::move(inner)} {}

public:
    [[nodiscard]]
    auto bind(const SocketAddr &addr) noexcept -> Result<void> {
        return inner_.bind<SocketAddr>(addr);
    }

    [[nodiscard]]
    auto listen(int n) -> Result<TcpListener> {
        if (auto ret = inner_.listen(n); !ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        return TcpListener{std::move(inner_)};
    }

    [[REMEMBER_CO_AWAIT]]
    auto connect(const SocketAddr &addr) {
        class Awaiter : public io::detail::IORegistrator<Awaiter> {
            using Super = io::detail::IORegistrator<Awaiter>;

        public:
            Awaiter(Socket &&inner, const SocketAddr &addr)
                : Super{io_uring_prep_connect, inner.fd(), nullptr, addr.length()}
                , inner_{std::move(inner)}
                , addr_{addr} {
                sqe_->addr = reinterpret_cast<unsigned long>(addr_.sockaddr());
            }

            auto await_resume() noexcept -> Result<TcpStream> {
                if (this->cb_.result_ < 0) [[unlikely]] {
                    return std::unexpected{make_sys_error(-this->cb_.result_)};
                }
                return TcpStream{std::move(inner_)};
            }

        private:
            Socket     inner_;
            SocketAddr addr_;
        };
        return Awaiter{std::move(inner_), addr};
    }

    [[nodiscard]]
    auto fd() const noexcept {
        return inner_.fd();
    }

public:
    [[nodiscard]]
    static auto v4() -> Result<TcpSocket> {
        return Socket::create<TcpSocket>(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    }

    [[nodiscard]]
    static auto v6() -> Result<TcpSocket> {
        return Socket::create<TcpSocket>(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    }

private:
    Socket inner_;
};

} // namespace zedio::socket::net
