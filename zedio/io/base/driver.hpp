#pragma once

#include "zedio/io/base/callback.hpp"
#include "zedio/io/base/io_uring.hpp"
#include "zedio/io/base/waker.hpp"
#include "zedio/runtime/queue.hpp"
#include "zedio/time/timer.hpp"
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

namespace zedio::io::detail {

class Driver;

thread_local Driver *t_driver{nullptr};

class Driver : util::Noncopyable {
public:
    Driver(const Config &config)
        : ring_{config} {
        assert(t_driver == nullptr);
        t_driver = this;
    }

    ~Driver() {
        t_driver = nullptr;
    }

    // Current worker thread will be blocked on io_uring_wait_cqe
    // until other worker wakes up it or a I/O event completes
    void wait(runtime::detail::LocalQueue  &local_queue,
              runtime::detail::GlobalQueue &global_queue) {
        ring_.wait(timer_.get_first_expired_time());
        poll(local_queue, global_queue);
    }

    auto poll(runtime::detail::LocalQueue &local_queue, runtime::detail::GlobalQueue &global_queue)
        -> bool {
        constexpr const auto             SIZE = Config::LOCAL_QUEUE_CAPACITY;
        std::array<io_uring_cqe *, SIZE> cqes;

        std::size_t cnt = ring_.peek_batch(cqes);
        for (auto i = 0uz; i < cnt; i += 1) {
            auto cb = reinterpret_cast<Callback *>(cqes[i]->user_data);
            if (cb != nullptr) [[likely]] {
                local_queue.push_back_or_overflow(cb->get_coro_handle_and_set_result(cqes[i]->res),
                                                  global_queue);
            }
        }

        ring_.cqe_advance(cnt);
        cnt += timer_.poll_batch(local_queue, global_queue);
        waker_.reg();
        ring_.force_submit();
        LOG_TRACE("poll {} events", cnt);
        return cnt > 0;
    }

    void wake_up() {
        return waker_.wake_up();
    }

private:
    IORing              ring_;
    Waker               waker_{};
    time::detail::Timer timer_{};
};

} // namespace zedio::io::detail
