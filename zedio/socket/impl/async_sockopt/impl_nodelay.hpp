#pragma once

#include "zedio/socket/impl/sockopt/async_sockopt.hpp"

namespace zedio::socket::detail {

template <class T>
struct ImplNodelay {
    [[REMEMBER_CO_AWAIT]]
    auto set_nodelay(bool on) noexcept {
        return setsockopt<int>(static_cast<T *>(this)->fd(),
                               SOL_TCP,
                               TCP_NODELAY,
                               static_cast<int>(on));
    }

    [[REMEMBER_CO_AWAIT]]
    auto nodelay() const noexcept {
        return getsockopt<int>(static_cast<const T *>(this)->fd(),
                               SOL_TCP,
                               TCP_NODELAY,
                               [](int optval) -> bool { return optval != 0; });
    }
};

} // namespace zedio::socket::detail
