#pragma once

#ifdef __linux__
#include "zedio/io/awaiter/linux/recv.hpp"
#include "zedio/io/awaiter/linux/recv_from.hpp"
#elif _WIN32
#include "zedio/io/awaiter/win/recv.hpp"
#include "zedio/io/awaiter/win/recv_from.hpp"
#endif

namespace zedio::io::detail {

template <class T, class Addr>
struct ImplDatagramRead {

    [[REMEMBER_CO_AWAIT]]
    auto recv(this const T &self, std::span<char> buf) noexcept {
        return Recv{self.handle(), buf.data(), buf.size_bytes(), 0};
    }

    [[REMEMBER_CO_AWAIT]]
    auto peek(this const T &self, std::span<char> buf) noexcept {
        return Recv{self.handle(), buf.data(), buf.size_bytes(), MSG_PEEK};
    }

    [[REMEMBER_CO_AWAIT]]
    auto recv_from(this const T &self, std::span<char> buf) noexcept {
        return RecvFrom{self.handle(), buf.data(), buf.size_bytes(), 0};
    }

    [[REMEMBER_CO_AWAIT]]
    auto peek_from(this const T &self, std::span<char> buf) noexcept {
        return RecvFrom{self.handle(), buf.data(), buf.size_bytes(), MSG_PEEK};
    }
};

} // namespace zedio::io::detail
