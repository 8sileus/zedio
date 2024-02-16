#pragma once

// C++
#include <concepts>
// Linux
#include <sys/socket.h>

namespace zedio {

template <typename Addr>
concept is_socket_address = requires(Addr addr) {
    { addr.sockaddr() } noexcept -> std::same_as<struct sockaddr *>;
    { addr.length() } noexcept -> std::same_as<socklen_t>;
};

} // namespace zedio
