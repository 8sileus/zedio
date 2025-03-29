#pragma once

#ifdef __linux__

#include "zedio/io/awaiter/linux/read.hpp"
#include "zedio/io/awaiter/linux/readv.hpp"
#define ReadFile read
#define ReadFileVectored readv

#elif _WIN32
#include "zedio/io/awaiter/win/read_file.hpp"
#endif

namespace zedio::socket::detail {

template <class T>
struct ImplFileRead {

    [[REMEMBER_CO_AWAIT]]
    auto read(this const T &self, std::span<char> buf) noexcept {
        return ReadFile{self.handle(), buf.data(), buf.size_bytes(), -1};
    }

#ifdef _WIN32
    template <typename Buf>
        requires constructible_to_char_slice<Buf>
    [[REMEMBER_CO_AWAIT]]
    auto read_vectored(this const T  &self,
                       std::span<Buf> bufs) noexcept -> zedio::async::Task<std::size_t> {
        std::size_t n = 0;
        for (auto &buf : bufs) {
            auto ret = co_await self.read(buf);
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
    auto read_vectored(this const T &self, std::span<Buf> bufs) noexcept {
        return ReadFileVectored{self.handle(), bufs};
    }
#endif

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
                co_return std::unexpected{make_zedio_error(Error::UnexpectedEOF)};
            }
            buf = buf.subspan(ret.value(), buf.size_bytes() - ret.value());
        }
        co_return Result<void>{};
    }
};

#ifdef __linux__

#undef ReadFile
#undef ReadVectoredFile

#endif

} // namespace zedio::socket::detail
