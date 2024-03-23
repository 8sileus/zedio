#pragma once

#include "zedio/socket/impl/sockopt/async_sockopt.hpp"

namespace zedio::socket::detail {

template <class T>
struct ImplBoradcast {
    [[REMEMBER_CO_AWAIT]]
    auto set_broadcast(bool on) noexcept {
        return setsockopt<int>(static_cast<T *>(this)->fd(),
                               SOL_SOCKET,
                               SO_BROADCAST,
                               static_cast<int>(on));
    }

    [[REMEMBER_CO_AWAIT]]
    auto broadcast() const noexcept {
        return getsockopt<int>(static_cast<const T *>(this)->fd(),
                               SOL_SOCKET,
                               SO_BROADCAST,
                               [](int optval) -> bool { return optval != 0; });
    }
};

} // namespace zedio::socket::detail
