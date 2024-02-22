#pragma once

#include "zedio/async/io.hpp"

namespace zedio::net::detail {

template <class Listener, class Stream, class Addr>
class BaseSocket {
protected:
    using IO = zedio::async::detail::IO;

    explicit BaseSocket(IO &&io)
        : io_{std::move(io)} {}

public:
    BaseSocket(BaseSocket &&other) = default;
    auto operator=(BaseSocket &&other) -> BaseSocket & = default;

    [[nodiscard]]
    auto bind(const Addr &address) const noexcept {
        return io_.bind<Addr>(address);
    }

    [[nodiscard]]
    auto fd() const noexcept {
        return io_.fd();
    }

    // Converts the socket into TcpListener
    [[nodiscard]]
    auto listen(int n) -> Result<Listener> {
        if (auto ret = io_.listen(n); !ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        return Listener{std::move(io_)};
    }

    // Converts the socket into TcpStream
    [[nodiscard]]
    auto connect(const Addr &addr) {
        class Awaiter : public async::detail::ConnectAwaiter<> {
        public:
            Awaiter(int fd, const Addr &addr)
                : ConnectAwaiter(fd, addr.sockaddr(), addr.length())
                , fd_{fd} {}

            auto await_resume() const noexcept -> Result<Stream> {
                auto ret = ConnectAwaiter::await_resume();
                if (!ret) [[unlikely]] {
                    return std::unexpected{ret.error()};
                }
                return Stream{IO::from_fd(fd_)};
            }

        private:
            int fd_;
        };
        return Awaiter{io_.take_fd(), addr};
    }

protected:
    IO io_;
};

} // namespace zedio::net::detail
