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
    auto is_distributable() const noexcept -> bool {
        return (state_ & DISTRIBUTABLE) == DISTRIBUTABLE;
    }

    void set_result(int result) {
        result_ = result;
        if (result >= 0) {
            state_ |= GOOD_RESULT;
        } else {
            state_ |= BAD_RESULT;
        }
    }

    [[nodiscard]]
    auto result() const noexcept -> int {
        return result_;
    }

    [[nodiscard]]
    auto is_ready() const noexcept -> bool {
        return (state_ & FINISHED_IO) != 0;
    }

    [[nodiscard]]
    auto is_null_sqe() const noexcept -> bool {
        return (state_ & NULL_SQE) != 0;
    }

    [[nodiscard]]
    auto is_good_result() const noexcept -> bool {
        return (state_ & GOOD_RESULT) != 0;
    }

    [[nodiscard]]
    auto is_bad_result() const noexcept -> bool {
        return (state_ & BAD_RESULT) != 0;
    }

    void set_null_sqe() {
        state_ |= NULL_SQE;
    }

private:
    static constexpr int DISTRIBUTABLE{static_cast<int>(OPFlag::Distributive)};
    static constexpr int GOOD_RESULT{1 << 3};
    static constexpr int BAD_RESULT{1 << 4};
    static constexpr int NULL_SQE{1 << 5};
    static constexpr int FINISHED_IO{GOOD_RESULT | BAD_RESULT | NULL_SQE};
};

} // namespace zed::async::detail
