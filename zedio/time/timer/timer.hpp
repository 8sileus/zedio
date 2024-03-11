#pragma once

#include "zedio/time/timer/wheel.hpp"

namespace zedio::time::detail {

constexpr inline std::size_t MAX_LEVEL = 7;

auto consteval pow64(std::size_t level) -> std::size_t {
    std::size_t res = 1;
    while (level--) {
        res *= SLOT_SIZE;
    }
    return res;
}

class Timer {
public:
    // auto add_entry() {}

    void remove_entry(Entry *entry) {
        wheel.remove_entry(entry);
    }

    auto next_expiration_time();

    auto handle_expired_entr() {
        std::vector<std::coroutine_handle<>> coros;
    }

private:
    std::chrono::steady_clock::time_point start_{std::chrono::steady_clock::now()};
    Wheel<pow64(MAX_LEVEL)>               wheel;
};

} // namespace zedio::time::detail
