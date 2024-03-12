#pragma once

#include "zedio/common/concepts.hpp"
#include "zedio/time/timer/timer.hpp"

using namespace std::chrono_literals;

namespace zedio::time {

namespace detail {

    class Sleep {
    public:
        explicit Sleep(std::chrono::steady_clock::time_point expired_time)
            : expired_time_{expired_time} {}

    public:
        [[nodiscard]]
        auto deadline() const noexcept -> std::chrono::steady_clock::time_point {
            return expired_time_;
        }

        auto await_ready() const noexcept -> bool {
            return false;
        }

        auto await_suspend(std::coroutine_handle<> handle) noexcept -> bool {
            auto ret = detail::t_timer->add_entry(expired_time_, handle);
            if (!ret) {
                result_ = std::unexpected{ret.error()};
                return false;
            }
            return true;
        }

        auto await_resume() const noexcept -> Result<void> {
            return result_;
        };

    private:
        std::chrono::steady_clock::time_point expired_time_;
        Result<void>                          result_;
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
