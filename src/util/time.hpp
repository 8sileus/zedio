#pragma once

// Linux
#include <sys/time.h>

// C++
#include <chrono>
#include <concepts>

namespace zed::util {

enum class Time {
    Hour,
    Minute,
    Second,
    MilliSecond,
};

template <Time T>
auto now() -> time_t {
    timeval tv;
    ::gettimeofday(&tv, nullptr);
    time_t milliseconds = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    if constexpr (T == Time::Hour) {
        return milliseconds / 60 / 60 / 1000;
    } else if constexpr (T == Time::Minute) {
        return milliseconds / 60 / 1000;
    } else if constexpr (T == Time::Second) {
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
