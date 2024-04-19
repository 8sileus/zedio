#pragma once

#include "zedio/socket/impl/sockopt/async_sockopt.hpp"

namespace zedio::socket::detail {

template <class T>
struct ImplMark {
    [[REMEMBER_CO_AWAIT]]
    auto set_mark(uint32_t mark) noexcept {
        return setsockopt<uint32_t>(static_cast<T *>(this)->fd(), SOL_SOCKET, SO_MARK, mark);
    }
};

} // namespace zedio::socket::detail
