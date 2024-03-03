#pragma once

#include "zedio/time/event.hpp"
// C++
#include <coroutine>
#include <set>

namespace zedio::io::detail {

struct Callback {
    // Why use it ,program will seg fault;
    // union {
    // std::coroutine_handle<> handle_{nullptr};
    // int                     result_;
    // };
    
    std::coroutine_handle<> handle_{nullptr};
    int                     result_{0};

    union {
        uint64_t                                has_timeout_{0};
        std::chrono::steady_clock::time_point   deadline_;
        std::set<time::detail::Event>::iterator iter_;
    };
};

} // namespace zedio::io::detail
