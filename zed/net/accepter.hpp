#pragma once

#include "async/async_io.hpp"
#include "common/exception.hpp"
#include "net/address.hpp"
// Linux
#include <sys/socket.h>
// C++
#include <concepts>

namespace zed::net {

enum class SocketType {
    TCP = SOCK_STREAM,
    UDP = SOCK_DGRAM,
};

template <SocketType socket_type>
class Accepter {
public:
    explicit Accepter(const Address &address)
        : address_(address)
        , fd_(::socket(address_.family(), socket_type, 0)) {
        if (fd_ < 0) {
            throw Exception("call socket failed", errno);
        }
        if (::bind(fd_, address_.get_sockaddr(), address_.get_length()) != 0) {
            throw Exception("call bind failed", errno);
        }
    }

    [[nodiscard]]
    auto accept() const -> Task<std::pair<int, Address>> {
        sockaddr  addr;
        socklen_t len;
        auto      fd = ::co_await async::Accept(fd_, &addr, &len, 0);
        co_return std::make_pair(fd, Address(&addr, &len));
    }

private:
    Address address_;
    int     fd_;
};

using TCPAccepter = Accepter<SocketType::TCP>;
using UDPAccepter = Accepter<SocketType::UDP>;

} // namespace zed::net
