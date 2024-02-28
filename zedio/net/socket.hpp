#pragma once

#include "zedio/io/io.hpp"

namespace zedio::net::detail {

template <class Listener, class Stream, class Addr>
class BaseSocket {
protected:
    using IO = zedio::io::IO;

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
        class Awaiter : public io::Connect {
        public:
            Awaiter(IO io, const Addr &addr)
                : Connect{io.fd(), addr.sockaddr(), addr.length()}
                , io_{std::move(io)} {}

            auto await_resume() const noexcept -> Result<Stream> {
                if (this->cb_.result_ >= 0) [[likely]] {
                    return Stream{std::move(io_)};
                } else {
                    return std::unexpected{make_sys_error(-this->cb_.result_)};
                }
            }
        private:
            IO io_;
        };
        return Awaiter{std::move(io_), addr};
    }

protected:
    IO io_;
};

} // namespace zedio::net::detail
