#pragma once

// C++
#include <chrono>
#include <coroutine>

namespace zedio::time::detail {

class Event {
public:
    Event(std::coroutine_handle<> handle, std::chrono::steady_clock::time_point expired_time)
        : handle_{handle}
        , expired_time_{expired_time} {}

    Event(int fd, std::chrono::steady_clock::time_point expired_time)
        : expired_time_{expired_time}
        , need_cancel_fd_{fd} {}

    auto operator<=>(const Event &other) const {
        if (this->expired_time_ == other.expired_time_) {
            return handle_.address() <=> other.handle_.address();
        }
        return this->expired_time_ <=> other.expired_time_;
    }

public:
    std::coroutine_handle<>               handle_{nullptr};
    std::chrono::steady_clock::time_point expired_time_;
    // if need_cancel_fd_ >= 0 , thie is a cancel tasks_
    int need_cancel_fd_{-1};
};

} // namespace zedio::time::detail
