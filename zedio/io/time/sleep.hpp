#pragma once

#include "zedio/common/concepts.hpp"
#include "zedio/io/time/timer.hpp"

using namespace std::chrono_literals;

namespace zedio::io {

class Sleep {
public:
    Sleep(const std::chrono::nanoseconds &timeout)
        : timeout_(timeout) {}

    auto await_ready() const noexcept -> bool {
        return false;
    }

    void await_suspend(std::coroutine_handle<> handle) const noexcept {
        auto cb = [handle]() { handle.resume(); };
        detail::t_timer->add_timer_event(cb, timeout_);
    }

    void await_resume() const noexcept {};

private:
    std::chrono::nanoseconds timeout_{};
};

} // namespace zedio::io
