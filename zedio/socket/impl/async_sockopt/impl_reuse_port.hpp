#pragma once

#include "zedio/socket/impl/sockopt/async_sockopt.hpp"

namespace zedio::socket::detail {

template <class T>
struct ImplReusePort {
    [[REMEMBER_CO_AWAIT]]
    auto set_reuseport(bool on) noexcept {
        return setsockopt<int>(static_cast<T *>(this)->fd(),
                               SOL_SOCKET,
                               SO_REUSEPORT,
                               static_cast<int>(on));
    }

    [[REMEMBER_CO_AWAIT]]
    auto reuseport() const noexcept {
        return getsockopt<int>(static_cast<const T *>(this)->fd(),
                               SOL_SOCKET,
                               SO_REUSEPORT,
                               [](int optval) -> bool { return optval != 0; });
    }
};

} // namespace zedio::socket::detail