#pragma once

#include "zedio/net/addr.hpp"
#include "zedio/net/datagram.hpp"

namespace zedio::net {

class UnixDatagram : public detail::BaseDatagram<UnixDatagram, UnixSocketAddr> {
    friend class BaseDatagram;

private:
    UnixDatagram(detail::SocketIO &&io)
        : BaseDatagram{std::move(io)} {}

public:
    [[nodiscard]]
    auto set_mark(uint32_t mark) noexcept {
        return io_.set_mark(mark);
    }

    [[nodiscard]]
    auto set_passcred(bool on) noexcept {
        return io_.set_passcred(on);
    }

    [[nodiscard]]
    auto passcred() const noexcept {
        return io_.passcred();
    }

public:
    [[nodiscard]]
    static auto unbound() -> Result<UnixDatagram> {
        auto io = detail::SocketIO::build_socket(AF_UNIX, SOCK_DGRAM, 0);
        if (!io) [[unlikely]] {
            return std::unexpected{io.error()};
        }
        return UnixDatagram{std::move(io.value())};
    }
};

} // namespace zedio::net
