#pragma once

#include "zedio/socket/impl/sockopt/async_sockopt.hpp"

namespace zedio::socket::detail {

template <class T>
struct ImplRecvBufferSize {
    [[REMEMBER_CO_AWAIT]]
    auto set_recv_buffer_size(uint32_t size) noexcept {
        return setsockopt<uint32_t>(static_cast<T *>(this)->fd(), SOL_SOCKET, SO_RCVBUF, size);
    }

    [[REMEMBER_CO_AWAIT]]
    auto recv_buffer_size() const noexcept {
        return getsockopt<uint32_t>(static_cast<const T *>(this)->fd(),
                                    SOL_SOCKET,
                                    SO_RCVBUF,
                                    [](uint32_t optval) -> uint32_t { return optval; });
    }
};

} // namespace zedio::socket::detail
