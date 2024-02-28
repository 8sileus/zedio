#pragma once

// C++
#include <coroutine>

namespace zedio::io::detail {

struct Callback {
    std::coroutine_handle<> handle_{nullptr};
    int                     result_{0};
    bool                    is_exclusive_{false};
};

} // namespace zedio::io::detail
