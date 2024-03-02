#pragma once

#include "zedio/common/macros.hpp"
#include "zedio/time/sleep.hpp"
// C++
#include <utility>

namespace zedio::time {

enum class MissedTickBehavior {
    Burst,
    Delay,
    Skip,
};

namespace detail {

    class Interval {
    public:
        Interval(std::chrono::steady_clock::time_point first_expired_time,
                 std::chrono::nanoseconds              period,
                 MissedTickBehavior                    behavior = MissedTickBehavior::Burst)
            : expired_time_{first_expired_time}
            , period_{period}
            , behavior_{behavior} {};

        [[REMEMBER_CO_AWAIT]]
        auto tick() noexcept -> Sleep {
            auto expired_time = expired_time_;
            expired_time_ = next_timeout();
            return Sleep{expired_time};
        }

        [[nodiscard]]
        auto period() const noexcept {
            return period_;
        }

        void reset() noexcept {
            expired_time_ = std::chrono::steady_clock::now() + period_;
        }

        void reset_immediately() noexcept {
            expired_time_ = std::chrono::steady_clock::now();
        }

        void reset_after(std::chrono::nanoseconds after) noexcept {
            expired_time_ = std::chrono::steady_clock::now() + after;
        }

        void reset_at(std::chrono::steady_clock::time_point deadline) noexcept {
            expired_time_ = deadline;
        }

        [[nodiscard]]
        auto missed_tick_behavior() const noexcept -> MissedTickBehavior {
            return behavior_;
        }

        void set_missed_tick_behavior(MissedTickBehavior behavior) noexcept {
            behavior_ = behavior;
        }

    private:
        auto next_timeout() -> std::chrono::steady_clock::time_point {
            auto now = std::chrono::steady_clock::now();
            switch (behavior_) {
            case MissedTickBehavior::Burst:
                return expired_time_ + period_;
            case MissedTickBehavior::Delay:
                return now + period_;
            case MissedTickBehavior::Skip:
                return now + period_
                       - std::chrono::nanoseconds{((now - expired_time_).count())
                                                  % period_.count()};
            }
            std::unreachable();
        }

    private:
        std::chrono::steady_clock::time_point expired_time_;
        std::chrono::nanoseconds              period_;
        MissedTickBehavior                    behavior_;
    };

} // namespace detail

static inline auto interval(std::chrono::nanoseconds duration) {
    return detail::Interval{std::chrono::steady_clock::now(), duration};
}

static inline auto interval_at(std::chrono::steady_clock::time_point start,
                               std::chrono::nanoseconds              duration) {
    return detail::Interval{start, duration};
}

} // namespace zedio::time
