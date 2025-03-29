#pragma once

#ifdef __linux__
#include "zedio/io/awaiter/linux/recv.hpp"
#elif _WIN32
#include "zedio/io/awaiter/win/recv.hpp"
#endif

namespace zedio::io::detail {

template <class T>
struct ImplStreamRead {

    [[REMEMBER_CO_AWAIT]]
    auto read(this const T &self, std::span<char> buf) noexcept {
        return Recv{self.handle(), buf.data(), buf.size_bytes(), 0};
    }

    [[REMEMBER_CO_AWAIT]]
    auto peek(this const T &self, std::span<char> buf) noexcept {
        return Recv{self.handle(), buf.data(), buf.size_bytes(), MSG_PEEK};
    }

    template <typename Buf>
        requires constructible_to_char_slice<Buf>
    [[REMEMBER_CO_AWAIT]]
    auto read_vectored(this const T &self, std::span<Buf> bufs) noexcept {
        return RecvVectored{self.handle(), bufs};
    }

    [[REMEMBER_CO_AWAIT]]
    auto read_exact(this const T   &self,
                    std::span<char> buf) noexcept -> zedio::async::Task<Result<void>> {
        Result<std::size_t> ret{0};
        while (!buf.empty()) {
            ret = co_await self.read(buf);
            if (!ret) [[unlikely]] {
                co_return std::unexpected{ret.error()};
            }
            if (ret.value() == 0) {
                co_return std::unexpected{make_error(Error::UnexpectedEOF)};
            }
            buf = buf.subspan(ret.value(), buf.size_bytes() - ret.value());
        }
        co_return Result<void>{};
    }
};

} // namespace zedio::io::detail
