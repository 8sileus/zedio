#pragma once

#include "zedio/io/io.hpp"

namespace zedio::net::detail {

template <class Stream, class Addr>
class BaseStream {
protected:
    using IO = zedio::io::IO;

    explicit BaseStream(IO &&io)
        : io_{std::move(io)} {}

public:
    BaseStream(BaseStream &&other) = default;
    auto operator=(BaseStream &&other) -> BaseStream & = default;

    [[nodiscard]]
    auto shutdown(io::Shutdown::How how) const noexcept {
        return io_.shutdown(how);
    }

    [[nodiscard]]
    auto close() noexcept {
        return io_.close();
    }

    [[nodiscard]]
    auto read(std::span<char> buf) const noexcept {
        return io_.recv(buf);
    }

    template <typename... Ts>
    [[nodiscard]]
    auto read_vectored(Ts &...bufs) const noexcept {
        return io_.read_vectored(bufs...);
    }

    [[nodiscard]]
    auto write(std::span<const char> buf) const noexcept {
        return io_.send(buf);
    }

    [[nodiscard]]
    auto write_all(std::span<const char> buf) const noexcept {
        // TODO use sendmsg
        return io_.write_all(buf);
    }

    template <typename... Ts>
    [[nodiscard]]
    auto write_vectored(Ts &...bufs) const noexcept {
        return io_.write_vectored(bufs...);
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
    static auto connect(const Addr &address) -> async::Task<Result<Stream>> {
        auto sock = IO::socket(address.family(), SOCK_STREAM, 0);
        if (!sock) [[unlikely]] {
            co_return std::unexpected{sock.error()};
        }
        if (auto ret = co_await sock.value().connect(address); !ret) [[unlikely]] {
            co_return std::unexpected{ret.error()};
        }
        co_return Stream{std::move(sock.value())};
    };

protected:
    IO io_;
};

} // namespace zedio::net::detail
