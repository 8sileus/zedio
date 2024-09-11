#pragma once

#include "zedio/common/debug.hpp"
#include "zedio/common/error.hpp"
#include "zedio/io/io.hpp"

// Linux
#include <unistd.h>

namespace zedio::io {

namespace detail {

    template <int FD>
    [[REMEMBER_CO_AWAIT]]
    auto async_read(std::span<char> buf) noexcept -> async::Task<Result<void>> {
        auto ret = co_await io::read(FD,
                                     buf.data(),
                                     static_cast<unsigned int>(buf.size_bytes()),
                                     static_cast<std::size_t>(-1));
        if (!ret) [[unlikely]] {
            co_return std::unexpected{ret.error()};
        }
        if (ret.value() == 0) [[unlikely]] {
            co_return std::unexpected{make_zedio_error(Error::UnexpectedEOF)};
        }
        co_return Result<void>{};
    }

    template <int FD>
    [[REMEMBER_CO_AWAIT]]
    auto async_write(std::span<const char> buf) noexcept -> async::Task<Result<void>> {
        Result<std::size_t> ret{0};
        while (!buf.empty()) {
            ret = co_await io::write(FD,
                                     buf.data(),
                                     static_cast<unsigned int>(buf.size_bytes()),
                                     static_cast<std::size_t>(-1));
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
} // namespace detail

[[REMEMBER_CO_AWAIT]]
static inline auto input(std::span<char> buf) -> async::Task<Result<void>> {
    co_return co_await detail::async_read<STDIN_FILENO>(buf);
}

template <typename... Args>
[[REMEMBER_CO_AWAIT]]
static inline auto print(std::string_view fmt, Args &&...args) -> async::Task<Result<void>> {
    co_return co_await detail::async_write<STDOUT_FILENO>(
        std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...)));
}

template <typename... Args>
[[REMEMBER_CO_AWAIT]]
static inline auto eprint(std::string_view fmt, Args &&...args) -> async::Task<Result<void>> {
    co_return co_await detail::async_write<STDERR_FILENO>(
        std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...)));
}

} // namespace zedio::io
