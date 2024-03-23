#pragma once

#include "zedio/socket/impl/sockopt/async_sockopt.hpp"

namespace zedio::socket::detail {

template <class T>
struct ImplTTL {
    [[REMEMBER_CO_AWAIT]]
    auto set_ttl(uint32_t ttl) noexcept {
        return setsockopt<uint32_t>(static_cast<T *>(this)->fd(), IPPROTO_IP, IP_TTL, ttl);
    }

    [[REMEMBER_CO_AWAIT]]
    auto ttl() const noexcept {
        return setsockopt<uint32_t>(static_cast<const T *>(this)->fd(),
                                    IPPROTO_IP,
                                    IP_TTL,
                                    [](uint32_t optval) -> uint32_t { return optval; });
    }
};

} // namespace zedio::socket::detail