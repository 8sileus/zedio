#pragma once

#include "zedio/time/event.hpp"
// C++
#include <coroutine>
#include <set>

namespace zedio::io::detail {

struct Callback {

    [[nodiscard]]
    auto get_coro_handle_and_set_result(int result) -> std::coroutine_handle<> {
        auto ret = handle_;
        result_ = result;
        return ret;
    }

    union {
        std::coroutine_handle<> handle_{nullptr};
        int                     result_;
    };

    union {
        uint64_t                                has_timeout_{0};
        std::chrono::steady_clock::time_point   deadline_;
        std::set<time::detail::Event>::iterator iter_;
    };
};

} // namespace zedio::io::detail
