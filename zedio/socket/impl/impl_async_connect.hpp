#pragma once

#include "zedio/io/awaiter/connect.hpp"

namespace zedio::io::detail {

template <class T, class Addr>
struct ImplAsyncConnect {
    [[REMEMBER_CO_AWAIT]]
    auto connect(const Addr &addr) noexcept {
        return io::detial::Connect{static_cast<const T *>(this)->fd(),
                                   addr.sockaddr(),
                                   addr.length()};
    }
};

} // namespace zedio::io::detail