#pragma once

// C++
#include <coroutine>

namespace zedio::io::detail {

struct Callback {
    std::coroutine_handle<> handle_{nullptr};
    int                     result_{0};
};

} // namespace zedio::io::detail
