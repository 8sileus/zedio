#pragma once

// Linux
#include <sys/time.h>

// C++
#include <chrono>
#include <concepts>

namespace zed::util {

enum class TimeUnit {
    Hour,
    Minute,
    Second,
    MilliSecond,
};

template <TimeUnit time_unit>
auto get_time_since_epoch() -> time_t {
    std::chrono::steady_clock::now().time_since_epoch();
    timeval tv;
    ::gettimeofday(&tv, nullptr);
    time_t milliseconds = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    if constexpr (time_unit == TimeUnit::Hour) {
        return milliseconds / 60 / 60 / 1000;
    } else if constexpr (time_unit == TimeUnit::Minute) {
        return milliseconds / 60 / 1000;
    } else if constexpr (time_unit == TimeUnit::Second) {
        return milliseconds / 1000;
    } else {
        return milliseconds;
    }
}

template <typename F, typename... Args>
    requires std::invocable<F, Args...>
auto spend_time(F &&f, Args &&...args) {
    auto start{std::chrono::steady_clock::now()};
    f(std::forward<Args>(args)...);
    auto end{std::chrono::steady_clock::now()};
    return end - start;
}

} // namespace zed::util
