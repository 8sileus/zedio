#pragma once

#include "zedio/common/macros.hpp"
#include "zedio/time/sleep.hpp"

namespace zedio::time {

namespace detail {

    class Interval {
    public:
        explicit Interval(Sleep start, std::chrono::nanoseconds period)
            : sleep_{start}
            , period_{period} {};

        [[REMEMBER_CO_AWAIT]]
        auto tick() noexcept -> Sleep {
            auto sleep = sleep_;
            sleep_.duration_ = period_;
            return sleep;
        }

    private:
        Sleep                    sleep_;
        std::chrono::nanoseconds period_;
    };

} // namespace detail

static inline auto interval(std::chrono::nanoseconds duration) {
    return detail::Interval{time::sleep(duration), duration};
}

static inline auto interval_at(std::chrono::steady_clock::time_point start,
                               std::chrono::nanoseconds              duration) {
    return detail::Interval{time::sleep_until(start), duration};
}

} // namespace zedio::time
