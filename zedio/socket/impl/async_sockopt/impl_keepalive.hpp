#pragma once

#include "zedio/socket/impl/sockopt/async_sockopt.hpp"

namespace zedio::socket::detail {

template <class T>
struct ImplKeepalive {
    [[REMEMBER_CO_AWAIT]]
    auto set_keepalive(bool on) noexcept {
        return setsockopt<int>(static_cast<T *>(this)->fd(),
                               SOL_TCP,
                               SO_KEEPALIVE,
                               static_cast<int>(on));
    }

    [[REMEMBER_CO_AWAIT]]
    auto keepalive() const noexcept {
        return getsockopt<int>(static_cast<const T *>(this)->fd(),
                               SOL_TCP,
                               SO_KEEPALIVE,
                               [](int optval) -> bool { return optval != 0; });
    }
};

} // namespace zedio::socket::detail
