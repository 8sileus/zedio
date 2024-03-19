#pragma once

// C++
#include <concepts>

namespace zedio::util {

template <typename T>
    requires std::is_integral_v<T>
static inline consteval auto static_pow(T x, T y) -> T {
    T result = 1;
    while (y--) {
        result *= x;
    }
    return result;
}

template <typename T>
    requires std::is_integral_v<T>
static inline consteval auto static_log(T x, T y) -> T {
    T result = 0;
    while (x != 1) {
        x /= y;
        result += 1;
    }
    return result;
}

} // namespace zedio::util