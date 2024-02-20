#pragma once

#include "zedio/async/io/awaiter.hpp"
#include "zedio/net/socket.hpp"

namespace zedio::net::detail {

template <class Stream, class Addr>
class Accepter : public async::detail::AcceptAwaiter<async::detail::Mode::S> {
    using return_type = Result<std::pair<Stream, Addr>>;

public:
    Accepter(int fd)
        : AcceptAwaiter(fd, reinterpret_cast<sockaddr *>(&addr_.addr_), &length_) {}

    auto await_resume() const noexcept -> return_type {
        auto ret = AcceptAwaiter::await_resume();
        if (!ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        return std::make_pair(Stream{Socket::from_fd(ret.value())}, addr_);
    }

private:
    Addr addr_{};
    socklen_t length_{};
};

} // namespace zedio::net::detail
