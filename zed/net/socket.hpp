#pragma once

#include "net/address.hpp"

//C
#include "cassert"

namespace zed::net {

[[nodiscard]]
auto get_local_address(int fd) -> Address {
    assert(fd >= 0);
    struct sockaddr_storage addr;
    ::memset(&addr, 0, sizeof(addr));
    socklen_t len;
    ::getsockname(fd, reinterpret_cast<struct sockaddr *>(&addr), &len);
    return Address(reinterpret_cast<struct sockaddr *>(&addr), len);
}

[[nodiscard]]
auto get_peer_address(int fd) -> Address {
    struct sockaddr_storage addr;
    ::memset(&addr, 0, sizeof(addr));
    socklen_t len;
    ::getpeername(fd, reinterpret_cast<struct sockaddr *>(&addr), &len);
    return Address(reinterpret_cast<struct sockaddr *>(&addr), len);
}

} // namespace zed::net
