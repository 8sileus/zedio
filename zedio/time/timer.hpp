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

thread_local Timer *t_timer;

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

    auto add_timer_event(void *data, std::chrono::steady_clock::time_point deadline) {
        auto [res, ok] = events_.emplace(data, deadline);
        assert(ok);
        return res;
    }

    auto add_timer_event(void *data, std::chrono::nanoseconds timeout) {
        return add_timer_event(data, std::chrono::steady_clock::now() + timeout);
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
            if (it->data_ == nullptr) {
                assert(it->handle_ != nullptr);
                local_queue.push_back_or_overflow(it->handle_, global_queue);
                cnt += 1;
            } else {
                auto sqe = io::detail::t_ring->get_sqe();
                assert(sqe != nullptr);
                reinterpret_cast<io::detail::Callback *>(it->data_)->has_timeout_ = 0;
                io_uring_prep_cancel(sqe, it->data_, 0);
                io_uring_sqe_set_data(sqe, nullptr);
            }
        }
        events_.erase(events_.begin(), it);
        return cnt;
    }

private:
    std::set<Event> events_{};
};

} // namespace zedio::time::detail
