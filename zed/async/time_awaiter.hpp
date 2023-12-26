#pragma once

#include "async/worker.hpp"
// C++
#include <chrono>
#include <coroutine>

namespace zed::detail::async {

struct Sleep {
    Sleep(const std::chrono::milliseconds &timeout)
        : timeout_(timeout) {}

    constexpr auto await_ready() const noexcept -> bool { return false; }

    template <typename T>
    void await_suspend(std::coroutine_handle<T> handle) const noexcept {
        auto task = [handle]() { handle.resume(); };
        detail::t_worker->add_timer_event(task, timeout_);
    }

    constexpr void await_resume() const noexcept {};

    std::chrono::milliseconds timeout_{};
};

} // namespace zed::detail::async