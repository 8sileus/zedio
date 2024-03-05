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

    Event(void *data, std::chrono::steady_clock::time_point expired_time)
        : data_{data}
        , expired_time_{expired_time} {}

    auto operator<=>(const Event &other) const {
        if (this->expired_time_ == other.expired_time_) {
            return this <=> std::addressof(other);
        }
        return this->expired_time_ <=> other.expired_time_;
    }

public:
    // TODO use std::function<void>;

    std::coroutine_handle<>               handle_{nullptr};
    void                                 *data_{nullptr};
    std::chrono::steady_clock::time_point expired_time_;
};

} // namespace zedio::time::detail
