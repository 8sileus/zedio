#pragma once

#include "zedio/net/io.hpp"

namespace zedio::net::detail {

template <class Listener, class Stream, class Addr>
class BaseListener {
protected:
    explicit BaseListener(SocketIO &&io)
        : io_{std::move(io)} {}

public:
    [[REMEMBER_CO_AWAIT]]
    auto accept() const noexcept {
        return io_.accept<Stream, Addr>();
    }

    [[nodiscard]]
    auto local_addr() const noexcept {
        return io_.local_addr<Addr>();
    }

    [[nodiscard]]
    auto fd() const noexcept {
        return io_.fd();
    }

    [[REMEMBER_CO_AWAIT]]
    auto close() noexcept {
        return io_.close();
    }

public:
    [[nodiscard]]
    static auto bind(const Addr &addr) -> Result<Listener> {
        return SocketIO::build_listener<Listener, Addr>(addr);
    }

    [[nodiscard]]
    static auto bind(const std::span<Addr> &addresses) -> Result<Listener> {
        for (const auto &address : addresses) {
            if (auto ret = bind(address); ret) [[likely]] {
                return ret;
            } else {
                LOG_ERROR("Bind {} failed, error: {}", address.to_string(), ret.error().message());
            }
        }
        return std::unexpected{make_zedio_error(Error::InvalidAddresses)};
    }

protected:
    SocketIO io_;
};

} // namespace zedio::net::detail
