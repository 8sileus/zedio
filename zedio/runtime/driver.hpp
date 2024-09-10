#pragma once

#include "zedio/io/base/io_data.hpp"
#include "zedio/runtime/io/io.hpp"
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
        : poller{config} {
        assert(t_driver == nullptr);
        t_driver = this;
    }

    ~Driver() {
        t_driver = nullptr;
    }

public:
    // Current worker thread will be blocked on io_uring_wait_cqe
    // until other worker wakes up it or a I/O event completes
    template <typename LocalQueue, typename GlobalQueue>
    void wait(this Driver &driver, LocalQueue &local_queue, GlobalQueue &global_queue) {
        driver.poller.wait(driver.timer.next_expiration_time());
        driver.poll(local_queue, global_queue);
    }

    template <typename LocalQueue, typename GlobalQueue>
    auto poll(this Driver &driver, LocalQueue &local_queue, GlobalQueue &global_queue) -> bool {
        constexpr const auto                   SIZE = LOCAL_QUEUE_CAPACITY;
        std::array<IOCompletion, SIZE>         completions;

        std::size_t cnt = driver.poller.peek_batch(completions);
        for (auto i = 0uz; i < cnt; i += 1) {
            auto io_data = completions[i].get_data();
            if (io_data != nullptr) [[likely]] {
                if (io_data->entry_ != nullptr) {
                    driver.timer.remove_entry(io_data->entry_);
                }
                io_data->result_ = completions[i].get_result();
                local_queue.push_back_or_overflow(io_data->handle_, global_queue);
            }
        }

        driver.poller.consume(cnt);

        auto timer_cnt = driver.timer.handle_expired_entries(local_queue, global_queue);

        LOG_TRACE("poll {} io events, {} timer events", cnt, timer_cnt);

        cnt += timer_cnt;

        driver.waker.turn_on();

        driver.poller.force_submit();

        return cnt > 0;
    }

    void wake_up(this Driver &driver) {
        return driver.waker.wake_up();
    }

private:
    Poller poller;
    Waker  waker{};
    Timer  timer{};
};

} // namespace zedio::runtime::detail
