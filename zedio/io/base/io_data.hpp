#pragma once

// C++
#include <chrono>
#include <coroutine>

namespace zedio::runtime::detail {

class Entry;

} // namespace zedio::runtime::detail

namespace zedio::io::detail {

struct IOData {
    std::coroutine_handle<>               handle_{nullptr};
    int                                   result_;
    runtime::detail::Entry               *entry_{nullptr};
    std::chrono::steady_clock::time_point deadline_;
};

} // namespace zedio::io::detail
