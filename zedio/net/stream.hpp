#pragma once

#include "zedio/net/io.hpp"

namespace zedio::net::detail {

template <class Stream, class Addr>
class BaseStream {
protected:
    BaseStream(detail::SocketIO &&io)
        : io_{std::move(io)} {}

public:
    [[REMEMBER_CO_AWAIT]]
    auto shutdown(io::ShutdownBehavior how) noexcept {
        return io_.shutdown(how);
    }

    [[REMEMBER_CO_AWAIT]]
    auto close() noexcept {
        return io_.close();
    }

    [[REMEMBER_CO_AWAIT]]
    auto read(std::span<char> buf) const noexcept {
        return io_.recv(buf);
    }

    [[REMEMBER_CO_AWAIT]]
    auto read_exact(std::span<char> buf) const noexcept {
        return io_.read_exact(buf);
    }

    template <typename... Ts>
    [[REMEMBER_CO_AWAIT]]
    auto read_vectored(Ts &&...bufs) const noexcept {
        return io_.read_vectored(std::forward<Ts>(bufs)...);
    }

    [[REMEMBER_CO_AWAIT]]
    auto write(std::span<const char> buf) noexcept {
        return io_.send(buf);
    }

    template <typename... Ts>
    [[REMEMBER_CO_AWAIT]]
    auto write_vectored(Ts &&...bufs) noexcept {
        return io_.write_vectored(std::forward<Ts>(bufs)...);
    }

    [[REMEMBER_CO_AWAIT]]
    auto write_all(std::span<const char> buf) noexcept {
        return io_.write_all(buf);
    }

    [[nodiscard]]
    auto local_addr() const noexcept {
        return io_.local_addr<Addr>();
    }

    [[nodiscard]]
    auto peer_addr() const noexcept {
        return io_.peer_addr<Addr>();
    }

    [[nodiscard]]
    auto fd() const noexcept {
        return io_.fd();
    }

public:
    [[nodiscard]]
    static auto connect(const Addr &addr) {
        return detail::SocketIO::build_stream<Stream, Addr>(addr);
    };

protected:
    detail::SocketIO io_;
};

} // namespace zedio::net::detail
