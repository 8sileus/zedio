#pragma once

#include "async/global_queue.hpp"
// system
#include <liburing.h>
// C++
#include <coroutine>

namespace zed::async {

struct IOAwaiter {
    IOAwaiter(io_uring_sqe *sqe, bool shared)
        : shared_(shared) {
        io_uring_sqe_set_data(sqe, static_cast<void *>(this));
    }

    constexpr auto await_ready() const noexcept -> bool { return false; }

    template <typename T>
    void await_suspend(std::coroutine_handle<T> handle) const noexcept {
        handle_ = handle;
    }

    constexpr auto await_resume() const noexcept -> int { return res_; }

    std::coroutine_handle<> handle_{nullptr};
    ssize_t                 res_{0};
    bool                    shared_{false};
};

struct YieldAwaiter{
    constexpr auto await_ready() const noexcept -> bool { return false; }

    template <typename T>
    void await_suspend(std::coroutine_handle<T> handle) const noexcept {
        detail::g_coros.push(handle);
    }

    const void await_resume() const noexcept {};
};

} // namespace zed::async