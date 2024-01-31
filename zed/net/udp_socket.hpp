#pragma once

#include "zed/net/socket.hpp"

namespace zed::net {

class UdpSocket {
private:
    UdpSocket(Socket &&sock)
        : socket_{std::move(sock)} {}

public:
       [[nodiscard]]
    auto send(std::span<const char> buf) const noexcept {
           return socket_.send(buf);
    }

    [[nodiscard]]
    auto try_send(std::span<const char> buf) const noexcept {
        return socket_.try_send(buf);
    }

    [[nodiscard]]
    auto send_to(std::span<const char> buf, const SocketAddr &addr) const noexcept {
        return socket_.send_to(buf, addr);
    }

    [[nodiscard]]
    auto try_send_to(std::span<const char> buf, const SocketAddr &addr) const noexcept {
        return socket_.try_send_to(buf, addr);
    }

    [[nodiscard]]
    auto recv(std::span<char> buf) const noexcept {
        return socket_.recv(buf);
    }

    [[nodiscard]]
    auto try_recv(std::span<char> buf) const noexcept {
        return socket_.try_recv(buf);
    }

    [[nodiscard]]
    auto set_broadcast(bool on)const noexcept{
        return socket_.set_broadcast(on);
    }

    [[nodiscard]]
    auto broadcast()const noexcept{
        return socket_.broadcast();
    }

public:
    [[nodiscard]]
    static auto bind(const SocketAddr &addr) -> Result<UdpSocket> {
        auto sock = Socket::build(addr.family(), SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP);
        if (!sock) [[unlikely]] {
            return std::unexpected{sock.error()};
        }
        if (auto ret = sock.value().bind(addr); !ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        return UdpSocket{std::move(sock.value())};
    }

private:
    Socket socket_;
};

} // namespace  zed::net
