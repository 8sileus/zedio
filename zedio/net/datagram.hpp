#pragma once

#include "zedio/io/io.hpp"

namespace zedio::net::detail {

template <class Datagram, class Addr>
class BaseDatagram {
protected:
    explicit BaseDatagram(SocketIO &&io)
        : io_{std::move(io)} {}

public:
    [[REMEMBER_CO_AWAIT]]
    auto send(std::span<const char> buf) noexcept {
        return io_.send(buf);
    }

    [[REMEMBER_CO_AWAIT]]
    auto send_to(std::span<const char> buf, const Addr &addr) noexcept {
        return io_.send_to<Addr>(buf, addr);
    }

    [[REMEMBER_CO_AWAIT]]
    auto recv(std::span<char> buf) const noexcept {
        return io_.recv(buf);
    }

    [[REMEMBER_CO_AWAIT]]
    auto recv_from(std::span<char>buf)const noexcept {
        return io_.recv_from<Addr>(buf);
    }

    [[REMEMBER_CO_AWAIT]]
    auto peek(std::span<char> buf) const noexcept {
        return io_.recv(buf, MSG_PEEK);
    }

    [[REMEMBER_CO_AWAIT]]
    auto peek_from(std::span<char> buf) const noexcept {
        return io_.peek_from<Addr>(buf);
    }

    [[REMEMBER_CO_AWAIT]]
    auto connect(Addr &addr) noexcept {
        return io_.connect<Addr>(addr);
    }

    [[nodiscard]]
    auto fd() const noexcept {
        return io_.fd();
    }

public:
    [[nodiscard]]
    static auto bind(const Addr &addr) -> Result<Datagram> {
        auto io = SocketIO::build_socket(addr.family(), SOCK_DGRAM, 0);
        if (!io) [[unlikely]] {
            return std::unexpected{io.error()};
        }
        if (auto ret = io.value().bind(addr); !ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        return Datagram{std::move(io.value())};
    }

protected:
    SocketIO io_;
};

} // namespace zedio::net::detail
