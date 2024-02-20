#pragma once

#include "zedio/common/error.hpp"
#include "zedio/common/util/noncopyable.hpp"
// C++
#include <coroutine>

namespace zedio::async::detail {

// Operation flag
enum class Mode : int32_t {
    // shared
    S = 1,
    // exclusive
    X = 2,
    // fixed fd
    F = 3,
};

struct Callback : util::Noncopyable {
public:
    Callback(Mode mode)
        : mode_{mode} {}

    [[nodiscard]]
    auto is_shared() -> bool {
        return mode_ == Mode::S;
    }

    std::coroutine_handle<> handle_;
    Mode                    mode_;
    int                     result_;

private:
    static constexpr int ASSIGNABLE = static_cast<int>(Mode::S);
    static constexpr int EXCLUSIVE{static_cast<int>(Mode::X)};
    static constexpr int FIXED{static_cast<int>(Mode::F)};
};

} // namespace zedio::async::detail
