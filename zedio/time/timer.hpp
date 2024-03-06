#pragma once

#include "zedio/common/debug.hpp"
#include "zedio/io/base/io_uring.hpp"
#include "zedio/runtime/queue.hpp"
#include "zedio/time/event.hpp"
// C++
#include <set>
#include <unordered_set>

namespace zedio::time::detail {

class Timer;

inline thread_local Timer *t_timer;

class Timer {
public:
    Timer() {
        assert(t_timer == nullptr);
        t_timer = this;
    }

    ~Timer() {
        t_timer = nullptr;
    }

    auto add_timer_event(std::coroutine_handle<>               handle,
                         std::chrono::steady_clock::time_point expired_time) {
        auto [res, ok] = events_.emplace(handle, expired_time);
        assert(ok);
        return res;
    }

    void cancel(std::set<Event>::iterator it) {
        events_.erase(it);
    }

    [[nodiscard]]
    auto get_first_expired_time() -> std::optional<std::chrono::nanoseconds> {
        if (events_.empty()) {
            return std::nullopt;
        } else {
            return events_.begin()->expired_time_ - std::chrono::steady_clock::now();
        }
    }

    auto poll_batch(runtime::detail::LocalQueue  &local_queue,
                    runtime::detail::GlobalQueue &global_queue) -> std::size_t {
        auto        now = std::chrono::steady_clock::now();
        auto        it = events_.begin();
        std::size_t cnt = 0;
        for (; it != events_.end() && it->expired_time_ <= now; ++it) {
            assert(it->handle_ != nullptr);
            local_queue.push_back_or_overflow(it->handle_, global_queue);
            cnt += 1;
        }
        events_.erase(events_.begin(), it);
        return cnt;
    }

private:
    std::set<Event> events_{};
};

} // namespace zedio::time::detail
