#pragma once

// C++
#include <coroutine>

namespace zed::async::detail {

// Operation flag
enum class OPFlag {
    // All thread can executes the coro
    Distributive = 1,
    // Only current thread can executes the coro
    Exclusive = 1 << 1,
    // Like Exclusive, but fd registered, will set IOSQE_FIXED_FILE
    Registered = 1 << 1 | 1 << 2,
};

struct BaseIOAwaiterData {
    std::coroutine_handle<> handle_;
    int                     state_;
    int                     result_{-1};

    BaseIOAwaiterData(int state)
        : state_{state} {}

    [[nodiscard]]
    auto is_distributable() const -> bool {
        return (state_ & DISTRIBUTABLE) == DISTRIBUTABLE;
    }

private:
    static constexpr int DISTRIBUTABLE{static_cast<int>(OPFlag::Distributive)};
};

} // namespace zed::async::detail
