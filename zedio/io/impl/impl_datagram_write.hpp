#pragma once

#ifdef __linux__
#include "zedio/io/awaiter/linux/send.hpp"
#include "zedio/io/awaiter/linux/send_to.hpp"
#elif _WIN32
#include "zedio/io/awaiter/win/send.hpp"
#include "zedio/io/awaiter/win/send_to.hpp"
#define MSG_NOSIGNAL 0
#endif

namespace zedio::io::detail {

template <class T, class Addr>
struct ImplDatagramWrite {
    [[REMEMBER_CO_AWAIT]]
    auto send(this T &self, std::span<const char> buf) noexcept {
        return Send{self.handle(), buf.data(), buf.size_bytes(), MSG_NOSIGNAL};
    }

    [[REMEMBER_CO_AWAIT]]
    auto send_to(this T &self, std::span<const char> buf, const Addr &addr) noexcept {
        return SendTo{self.handle(),
                      buf.data(),
                      buf.size_bytes(),
                      MSG_NOSIGNAL,
                      addr.sockaddr(),
                      addr.length()};
    }
};

#ifdef _WIN32
#undef MSG_NOSIGNAL
#endif

} // namespace zedio::io::detail
