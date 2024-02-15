#pragma once

#include "zedio/net/socket.hpp"
#include "zedio/net/unix/addr.hpp"

//C++
namespace zedio::net {

class UnixStream {
    friend class UnixListener;
    friend class UnixSocket;

private:
    explicit UnixStream(Socket &&io)
        : io_{std::move(io)} {}

public:
    UnixStream(UnixStream &&other) noexcept
        : io_{std::move(other.io_)} {}

    [[nodiscard]]
    auto local_addr() const noexcept {
        return io_.local_addr<UnixSocketAddr>();
    }

    [[nodiscard]]
    auto peer_addr() const noexcept {
        return io_.peer_addr<UnixSocketAddr>();
    }

    [[nodiscard]]
    auto fd() const noexcept {
        return io_.fd();
    }

    [[nodiscard]]
    auto read(std::span<char> buf) const noexcept {
        return io_.read(buf);
    }

    template <typename... Ts>
    [[nodiscard]]
    auto read_vectored(Ts &...bufs) {
        return io_.read_vectored(bufs...);
    }

    [[nodiscard]]
    auto write(std::span<const char> buf) const noexcept {
        return io_.write(buf);
    }

    [[nodiscard]]
    auto write_all(std::span<const char> buf) const noexcept {
        return io_.write_all(buf);
    }

    template <typename... Ts>
    [[nodiscard]]
    auto write_vectored(Ts &...bufs) const noexcept {
        return io_.write_vectored(bufs...);
    }

public:
    [[nodiscard]]
    static auto connect(const UnixSocketAddr &address) -> async::Task<Result<UnixStream>> {
        auto io = Socket::build(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (!io) [[unlikely]] {
            co_return std::unexpected{io.error()};
        }
        if (auto ret = co_await io.value().connect<UnixSocketAddr>(address); !ret) [[unlikely]] {
            co_return std::unexpected{ret.error()};
        }
        co_return UnixStream{std::move(io.value())};
    }

private:
    Socket io_;
};

} // namespace zedio::net
