#pragma once

#include "zedio/socket/impl/sockopt/async_sockopt.hpp"

namespace zedio::socket::detail {

template <class T>
struct ImplLinger {
    [[nodiscard]]
    auto set_linger(std::optional<std::chrono::seconds> duration) noexcept {
        struct linger lin {
            .l_onoff{0}, .l_linger{0},
        };
        if (duration.has_value()) {
            lin.l_onoff = 1;
            lin.l_linger = duration.value().count();
        }
        return setsockopt<struct linger>(static_cast<T *>(this)->fd(), SOL_SOCKET, SO_LINGER, lin);
    }

    [[nodiscard]]
    auto linger() const noexcept {
        return getsockopt<struct linger>(
            static_cast<const T *>(this)->fd(),
            SOL_SOCKET,
            SO_LINGER,
            [](struct linger lin) -> std::optional<std::chrono::seconds> {
                if (lin.l_onoff == 0) {
                    return std::nullopt;
                } else {
                    return std::chrono::seconds(lin.l_linger);
                }
            });
    }
};

} // namespace zedio::socket::detail