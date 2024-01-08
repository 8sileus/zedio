// TODO optimize the queue ,maybe unlock?

#pragma once

#include "zed/util/thread.hpp"
// C++
#include <mutex>
#include <optional>
#include <queue>

namespace zed {

template <typename T>
class SafeQueue {
public:
    void push(const T &val) {
        std::lock_guard lock(mutex_);
        data_.push(val);
    }

    [[nodiscard]]
    auto take() -> std::optional<T> {
        std::lock_guard lock(mutex_);
        if (data_.empty()) {
            return std::nullopt;
        }
        auto res = std::move(data_.front());
        data_.pop();
        return res;
    }

private:
    std::queue<T>   data_;
    util::SpinMutex mutex_;
};

} // namespace zed