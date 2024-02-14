#pragma once

// C++
#include <chrono>
#include <concepts>

namespace zedio::util {

template <typename F, typename... Args>
    requires std::invocable<F, Args...>
auto spend_time(F &&f, Args &&...args) {
    auto start{std::chrono::steady_clock::now()};
    f(std::forward<Args>(args)...);
    auto end{std::chrono::steady_clock::now()};
    return end - start;
}

} // namespace zedio::util
