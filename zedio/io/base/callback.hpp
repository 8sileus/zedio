#pragma once

// C++
#include <chrono>
#include <coroutine>

namespace zedio::runtime::detail {

class Entry;

} // namespace zedio::runtime::detail

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
        runtime::detail::Entry               *entry_{nullptr};
        std::chrono::steady_clock::time_point deadline_;
    };
};

} // namespace zedio::io::detail
