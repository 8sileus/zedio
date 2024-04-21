#pragma once

#include "zedio/io/base/callback.hpp"
#include "zedio/runtime/io/io_uring.hpp"
#include "zedio/runtime/io/waker.hpp"
#include "zedio/runtime/queue.hpp"
#include "zedio/runtime/timer/timer.hpp"
// C
#include <cstring>
// C++
#include <chrono>
#include <expected>
#include <format>
#include <functional>
#include <numeric>
#include <queue>
// Linux
#include <liburing.h>
#include <sys/eventfd.h>

namespace zedio::runtime::detail {

class Driver;

inline thread_local Driver *t_driver{nullptr};

class Driver {
public:
    Driver(const Config &config)
        : ring_{config} {
        assert(t_driver == nullptr);
        t_driver = this;
    }

    ~Driver() {
        t_driver = nullptr;
    }

public:
    // Current worker thread will be blocked on io_uring_wait_cqe
    // until other worker wakes up it or a I/O event completes
    template <typename Q>
        requires requires(Q q, std::coroutine_handle<> h) {
            { q.push(h) };
        }
    void wait(Q &queue) {
        ring_.wait(timer_.next_expiration_time());
        poll(queue);
    }

    template <typename Q>
        requires requires(Q q, std::coroutine_handle<> h) {
            { q.push(h) };
        }
    auto poll(Q &queue) -> bool {
        constexpr const auto             SIZE = LOCAL_QUEUE_CAPACITY;
        std::array<io_uring_cqe *, SIZE> cqes;

        std::size_t cnt = ring_.peek_batch(cqes);
        for (auto i = 0uz; i < cnt; i += 1) {
            auto cb = reinterpret_cast<io::detail::Callback *>(cqes[i]->user_data);
            if (cb != nullptr) [[likely]] {
                if (cb->entry_ != nullptr) {
                    timer_.remove_entry(cb->entry_);
                }
                queue.push(cb->get_coro_handle_and_set_result(cqes[i]->res));
            }
        }

        ring_.consume(cnt);

        auto timer_cnt = timer_.handle_expired_entries(queue);

        LOG_TRACE("poll {} io events, {} timer events", cnt, timer_cnt);

        cnt += timer_cnt;

        waker_.turn_on();

        ring_.force_submit();

        return cnt > 0;
    }

    void wake_up() {
        return waker_.wake_up();
    }

private:
    IORing ring_;
    Waker  waker_{};
    Timer  timer_{};
};

} // namespace zedio::runtime::detail
