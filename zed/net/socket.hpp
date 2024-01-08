#pragma once

#include "zed/net/socket_addr.hpp"
// C
#include "cassert"

namespace zed::net {

[[nodiscard]]
inline auto get_local_address(int fd) -> std::expected<SocketAddr, std::error_code> {
    assert(fd >= 0);
    struct sockaddr_storage addr {};
    socklen_t               len;
    if (::getsockname(fd, reinterpret_cast<struct sockaddr *>(&addr), &len) != 0) {
        return std::unexpected{
            std::error_code{errno, std::system_category()}
        };
    } // namespace zed::net
    return SocketAddr(reinterpret_cast<struct sockaddr *>(&addr), len);
}

[[nodiscard]]
inline auto get_peer_address(int fd) -> std::expected<SocketAddr, std::error_code> {
    struct sockaddr_storage addr {};
    socklen_t               len;
    if (::getpeername(fd, reinterpret_cast<struct sockaddr *>(&addr), &len) != 0) {
        return std::unexpected{
            std::error_code{errno, std::system_category()}
        };
    }
    return SocketAddr(reinterpret_cast<struct sockaddr *>(&addr), len);
}

[[nodiscard]]
inline auto set_sock_opt(int fd, int level, int optname, const void *optval, socklen_t optlen)
    -> std::expected<void, std::error_code> {
    if (::setsockopt(fd, level, optname, optval, optlen) != 0) [[unlikely]] {
        return std::unexpected{
            std::error_code{errno, std::system_category()}
        };
    }
    return {};
}

} // namespace zed::net
