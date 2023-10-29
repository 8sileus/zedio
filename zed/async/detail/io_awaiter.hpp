#pragma once

// Linux
#include <liburing.h>
// C
#include <cassert>
// C++
#include <coroutine>
#include <functional>

namespace zed::async::detail {

struct LazyBaseIOAwaiter {
    LazyBaseIOAwaiter(pid_t tid, const std::function<void(io_uring_sqe *)> &cb)
        : tid_(tid)
        , cb_(cb) {}

    virtual ~LazyBaseIOAwaiter() = default;

    constexpr auto await_ready() const noexcept -> bool { return false; }

    void await_suspend(std::coroutine_handle<> handle);

    constexpr auto await_resume() const noexcept -> int { return res_; }

    pid_t                               tid_{0};
    int                                 res_{0};
    std::function<void(io_uring_sqe *)> cb_;
    std::coroutine_handle<>             handle_{};
};

} // namespace zed::async::detail
