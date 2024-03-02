#pragma once

#include "zedio/common/concepts.hpp"
#include "zedio/time/timer.hpp"

using namespace std::chrono_literals;

namespace zedio::time {

namespace detail {

    class Sleep {
    public:
        explicit Sleep(std::chrono::steady_clock::time_point expired_time)
            : expired_time_{expired_time} {}

        auto await_ready() const noexcept -> bool {
            return false;
        }

        auto await_suspend(std::coroutine_handle<> handle) noexcept {
            if (expired_time_ > std::chrono::steady_clock::now() + 1ms) {
                t_timer->add_timer_event(handle, expired_time_);
                return true;
            } else {
                return false;
            }
        }

        void await_resume() const noexcept {};

        [[nodiscard]]
        auto deadline() const noexcept -> std::chrono::steady_clock::time_point {
            return expired_time_;
        }

    private:
        std::chrono::steady_clock::time_point expired_time_;
    };

} // namespace detail

[[REMEMBER_CO_AWAIT]]
static inline auto sleep(const std::chrono::nanoseconds &duration) {
    return detail::Sleep{std::chrono::steady_clock::now() + duration};
}

[[REMEMBER_CO_AWAIT]]
static inline auto sleep_until(std::chrono::steady_clock::time_point expired_time) {
    return detail::Sleep{expired_time};
}

} // namespace zedio::time
