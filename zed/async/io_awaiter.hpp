#pragma once

// Linux
#include <liburing.h>
// C
#include <cassert>
// C++
#include <coroutine>
#include <expected>
#include <functional>
#include <system_error>

namespace zed::async::detail {

// Access Level
enum class AL : int8_t {
    shared,
    privated,
};

struct BaseIOAwaiter {
    BaseIOAwaiter(AL level)
        : level_{level} {}

    auto await_ready() const noexcept -> bool {
        return ready_;
    }

    void await_suspend(std::coroutine_handle<> handle);

    constexpr auto await_resume() const noexcept -> std::expected<int, std::error_code> {
        if (res_ >= 0) [[likely]] {
            return res_;
        }
        return std::unexpected{
            std::error_code{-res_, std::system_category()}
        };
    }

    int                                 res_{0};
    bool                                ready_{false};
    AL                                  level_;
    std::coroutine_handle<>             handle_{};
    std::function<void(io_uring_sqe *)> cb_{nullptr};
};

} // namespace zed::async::detail
