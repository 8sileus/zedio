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
        : inner{std::move(inner)} {}

public:
    [[nodiscard]]
    auto bind(this TcpSocket &self, const SocketAddr &addr) noexcept -> Result<void> {
        return self.inner.bind<SocketAddr>(addr);
    }

    [[nodiscard]]
    auto listen(this TcpSocket &self, int n) noexcept -> Result<TcpListener> {
        if (auto result = self.inner.listen(n); !result) [[unlikely]]
        {
            return std::unexpected{result.error()};
        }
        return TcpListener{std::move(self.inner)};
    }

    [[REMEMBER_CO_AWAIT]]
    auto connect(this TcpSocket &self, const SocketAddr &addr) {
        class Connect : public io::detail::Connect {
        public:
            Connect(Socket &&inner, const SocketAddr &addr)
                : io::detail::Connect{inner.handle(), addr.sockaddr(), addr.length()}
                , inner{std::move(inner)} {}

            auto await_resume(this Connect &self) noexcept -> Result<TcpStream> {
                auto result = self.io::detail::Connect::await_resume();
                if (!result) [[unlikely]] {
                    return std::unexpected{result.error()};
                }
                return TcpStream{std::move(self.inner)};
            }

        private:
            Socket inner;
        };
        return Connect{std::move(self.inner), addr};
    }

    [[nodiscard]]
    auto handle(this const TcpSocket &self) noexcept {
        return self.inner.handle();
    }

public:
    [[nodiscard]]
    static auto v4() -> Result<TcpSocket> {
        return Socket::create<TcpSocket>(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }

    [[nodiscard]]
    static auto v6() -> Result<TcpSocket> {
        return Socket::create<TcpSocket>(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    }

private:
    Socket inner;
};

} // namespace zedio::socket::net
