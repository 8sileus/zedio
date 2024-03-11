#pragma once

// C++
#include <chrono>
#include <coroutine>
#include <memory>

namespace zedio::time::detail {

struct Entry {
    std::chrono::milliseconds ms_;
    std::coroutine_handle<>   handle_{nullptr};
    std::unique_ptr<Entry>    next_{nullptr};
};

} // namespace zedio::time::detail
