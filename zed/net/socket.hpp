#pragma once

#include "zed/net/socket_addr.hpp"
// C
#include "cassert"

namespace zed::net {

[[nodiscard]]
static inline auto get_local_address(int fd) -> std::expected<SocketAddr, std::error_code> {
    assert(fd >= 0);
    struct sockaddr_storage addr {};
    socklen_t               len;
    if (::getsockname(fd, reinterpret_cast<struct sockaddr *>(&addr), &len) != 0) {
        return std::unexpected{make_sys_error(errno)};
    } // namespace zed::net
    return SocketAddr(reinterpret_cast<struct sockaddr *>(&addr), len);
}

[[nodiscard]]
static inline auto get_peer_address(int fd) -> std::expected<SocketAddr, std::error_code> {
    struct sockaddr_storage addr {};
    socklen_t               len;
    if (::getpeername(fd, reinterpret_cast<struct sockaddr *>(&addr), &len) != 0) {
        return std::unexpected{make_sys_error(errno)};
    }
    return SocketAddr(reinterpret_cast<struct sockaddr *>(&addr), len);
}

[[nodiscard]]
static inline auto set_sock_opt(int fd, int level, int optname, const void *optval,
                                socklen_t optlen) -> std::expected<void, std::error_code> {
    if (::setsockopt(fd, level, optname, optval, optlen) != 0) [[unlikely]] {
        return std::unexpected{make_sys_error(errno)};
    }
    return {};
}

[[nodiscard]]
static inline auto get_sock_opt(int fd, int level, int optname, void *optval, socklen_t optlen)
    -> std::expected<void, std::error_code> {
    auto real_len = optlen;
    if (::getsockopt(fd, level, optname, optval, &real_len) != 0 || real_len != optlen)
        [[unlikely]] {
        return std::unexpected{make_sys_error(errno)};
    }
    return {};
}

} // namespace zed::net
