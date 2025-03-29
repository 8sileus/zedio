#pragma once

#ifdef __linux__
#include "zedio/io/awaiter/linux/send.hpp"
#elif _WIN32
#include "zedio/io/awaiter/win/send.hpp"
#define MSG_NOSIGNAL 0
#endif

namespace zedio::io::detail {

template <class T>
struct ImplStreamWrite {

    [[REMEMBER_CO_AWAIT]]
    auto write(this T &self, std::span<const char> buf) noexcept {
        return Send{self.handle(), buf.data(), buf.size_bytes(), MSG_NOSIGNAL};
    }

    template <typename Buf>
        requires constructible_to_char_slice<Buf>
    [[REMEMBER_CO_AWAIT]]
    auto write_vectored(this T &self, std::span<Buf> bufs) noexcept {
        return SendVectored{self.handle(), bufs, MSG_NOSIGNAL};
    }

    [[REMEMBER_CO_AWAIT]]
    auto write_all(this T               &self,
                   std::span<const char> buf) noexcept -> zedio::async::Task<Result<void>> {
        Result<std::size_t> ret{0};
        while (!buf.empty()) {
            ret = co_await self.write(buf);
            if (!ret) [[unlikely]] {
                co_return std::unexpected{ret.error()};
            }
            if (ret.value() == 0) [[unlikely]] {
                co_return std::unexpected{make_error(Error::WriteZero)};
            }
            buf = buf.subspan(ret.value(), buf.size_bytes() - ret.value());
        }
        co_return Result<void>{};
    }
};

#ifdef _WIN32
#undef MSG_NOSIGNAL
#endif

} // namespace zedio::io::detail
