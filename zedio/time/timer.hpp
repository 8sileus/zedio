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

    auto add_timer_event(std::coroutine_handle<> handle, std::chrono::nanoseconds duration) {
        auto [res, ok] = events_.emplace(handle, std::chrono::steady_clock::now() + duration);
        assert(ok);
        return res;
    }

    auto add_timer_event(int fd, std::chrono::nanoseconds duration) {
        auto [res, ok] = events_.emplace(fd, std::chrono::steady_clock::now() + duration);
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

    void handle_expired_events(runtime::detail::LocalQueue  &local_queue,
                               runtime::detail::GlobalQueue &global_queue) {
        auto now = std::chrono::steady_clock::now();
        auto it = events_.begin();
        for (; it != events_.end() && it->expired_time_ <= now; ++it) {
            if (it->need_cancel_fd_ == -1) {
                local_queue.push_back_or_overflow(it->handle_, global_queue);
            } else {
                auto sqe = io::detail::t_ring->get_sqe();
                assert(sqe != nullptr);
                io_uring_prep_cancel_fd(sqe, it->need_cancel_fd_, 0);
                io_uring_sqe_set_data(sqe, nullptr);
            }
        }
        events_.erase(events_.begin(), it);
    }

    void handle_expired_events(std::optional<std::coroutine_handle<>> &run_next) {
         auto now = std::chrono::steady_clock::now();
        auto it = events_.begin();
        for (; it != events_.end() && it->expired_time_ <= now; ++it) {
            if (it->need_cancel_fd_ == -1) {
                run_next = it->handle_;
                ++it;
                break;
            } else {
                auto sqe = io::detail::t_ring->get_sqe();
                assert(sqe != nullptr);
                io_uring_prep_cancel_fd(sqe, it->need_cancel_fd_, 0);
                io_uring_sqe_set_data(sqe, nullptr);
            }
        }
        events_.erase(events_.begin(), it);
    }

private:
    std::set<Event> events_{};
};

} // namespace zedio::time::detail
