#pragma once

#ifdef __linux__
#include "zedio/io/awaiter/linux/write.hpp"
#include "zedio/io/awaiter/linux/writev.hpp"
#elif _WIN32
#include "zedio/io/awaiter/win/write_file.hpp"
#endif


namespace zedio::io::detail {

template <class T>
struct ImplFileWrite {

    [[REMEMBER_CO_AWAIT]]
    auto write(this T &self, std::span<const char> buf) noexcept {
        return WriteFile{self.handle(), buf.data(), buf.size_bytes(), -1};
    }


#ifdef _WIN32
    template <typename Buf>
        requires constructible_to_char_slice<Buf>
    [[REMEMBER_CO_AWAIT]]
    auto write_vectored(this const T  &self,
                        std::span<Buf> bufs) noexcept -> zedio::async::Task<std::size_t> {
        std::size_t n = 0;
        for (auto &buf : bufs) {
            auto ret = co_await self.write(buf);
            if (!ret || ret.value() == 0) {
                break;
            }
            n += ret.value();
        }
        co_return n;
    }
#elif __linux__

    template <typename Buf>
        requires constructible_to_char_slice<Buf>
    [[REMEMBER_CO_AWAIT]]
    auto write_vectored(this T &self, std::span<Buf> bufs) noexcept {
        return WriteFileVectored{self.handle(), bufs, MSG_NOSIGNAL};
    }
#endif

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
                co_return std::unexpected{make_zedio_error(Error::WriteZero)};
            }
            buf = buf.subspan(ret.value(), buf.size_bytes() - ret.value());
        }
        co_return Result<void>{};
    }
};

} // namespace zedio::io::detail
