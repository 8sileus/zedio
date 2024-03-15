#pragma once

#include "zedio/io/awaiter/send.hpp"

namespace zedio::socket::detail {

template <class T, class Addr>
struct ImplAsyncSend {
    [[REMEMBER_CO_AWAIT]]
    auto send(std::span<const char> buf) noexcept {
        return io::detail::Send{static_cast<T *>(this)->fd(),
                                buf.data(),
                                buf.size_bytes(),
                                MSG_NOSIGNAL};
    }

    [[REMEMBER_CO_AWAIT]]
    auto send_to(std::span<const char> buf, const Addr &addr) noexcept {
        return io::detail::SendTo{static_cast<T *>(this)->fd(),
                                  buf.data(),
                                  buf.size_bytes(),
                                  MSG_NOSIGNAL,
                                  addr.sockaddr(),
                                  addr.length()};
    }
};

} // namespace zedio::socket::detail
