#pragma once

#include "zedio/io/io.hpp"

namespace zedio::net::detail {

template <class Datagram, class Addr>
class BaseDatagram {
protected:
    using IO = zedio::io::IO;

    explicit BaseDatagram(IO &&io)
        : io_{std::move(io)} {}

public:
    BaseDatagram(BaseDatagram &&) = default;

    [[nodiscard]]
    auto send(std::span<const char> buf) const noexcept {
        return io_.send(buf);
    }

    [[nodiscard]]
    auto send_to(std::span<const char> buf, const Addr &addr) const noexcept {
        return io_.send_to<Addr>(buf, addr);
    }

    [[nodiscard]]
    auto recv(std::span<char> buf) const noexcept {
        return io_.recv(buf);
    }

    [[nodiscard]]
    auto peek(std::span<char> buf) const noexcept {
        return io_.recv(buf, MSG_PEEK);
    }

    auto connect(Addr &addr) const noexcept {
        return io_.connect<Addr>(addr);
    }

public:
    [[nodiscard]]
    static auto bind(const Addr &addr) -> Result<Datagram> {
        auto io = IO::socket(addr.family(), SOCK_DGRAM, 0);
        if (!io) [[unlikely]] {
            return std::unexpected{io.error()};
        }
        if (auto ret = io.value().bind(addr); !ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        return Datagram{std::move(io.value())};
    }

protected:
    IO io_;
};

} // namespace zedio::net::detail
