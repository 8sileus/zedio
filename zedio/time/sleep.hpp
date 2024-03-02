#pragma once

#include "zedio/common/concepts.hpp"
#include "zedio/time/timer.hpp"

using namespace std::chrono_literals;

namespace zedio::time {

namespace detail {

    class Sleep {
        friend class Interval;

    public:
        explicit Sleep(const std::chrono::nanoseconds &duration)
            : duration_(duration) {}

        auto await_ready() const noexcept -> bool {
            return false;
        }

        void await_suspend(std::coroutine_handle<> handle) noexcept {
            t_timer->add_timer_event(handle, duration_);
        }

        void await_resume() const noexcept {};

    private:
        std::chrono::nanoseconds duration_{};
    };

} // namespace detail

[[REMEMBER_CO_AWAIT]]
static inline auto sleep(std::chrono::nanoseconds duration) {
    return detail::Sleep{duration};
}

[[REMEMBER_CO_AWAIT]]
static inline auto sleep_until(std::chrono::steady_clock::time_point start) {
    return detail::Sleep{start - std::chrono::steady_clock::now()};
}

} // namespace zedio::time
