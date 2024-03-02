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
    void wait(std::optional<std::coroutine_handle<>> &run_next) {
        assert(!run_next.has_value());

        auto cqe = ring_.wait_cqe(timer_.get_first_expired_time());
        if (cqe == nullptr) {
            timer_.handle_expired_events(run_next);
            t_ring->force_submit();
            return;
        }

        auto cb = reinterpret_cast<Callback *>(cqe->user_data);
        if (cb != nullptr) [[likely]] {
            cb->result_ = cqe->res;
            run_next = cb->handle_;
        }
        ring_.cqe_advance(1);
    }

    [[nodiscard]]
    auto poll(runtime::detail::LocalQueue &local_queue, runtime::detail::GlobalQueue &global_queue)
        -> bool {
        constexpr const auto                 SIZE = Config::LOCAL_QUEUE_CAPACITY;
        std::array<io_uring_cqe *, SIZE * 2> cqes;

        std::size_t cnt = ring_.peek_batch(cqes);
        if (cnt == 0) {
            wait_before();
            return false;
        }
        for (auto i = 0uz; i < cnt; i += 1) {
            auto cb = reinterpret_cast<Callback *>(cqes[i]->user_data);
            if (cb != nullptr) [[likely]] {
                cb->result_ = cqes[i]->res;
                local_queue.push_back_or_overflow(cb->handle_, global_queue);
            }
        }
        ring_.cqe_advance(cnt);
        timer_.handle_expired_events(local_queue, global_queue);
        ring_.force_submit();
        LOG_TRACE("poll {} events", cnt);
        return true;
    }

    void push_waiting_coro(std::function<void(io_uring_sqe *sqe)> &&cb) {
        LOG_DEBUG("push a waiting coros");
        waiting_coros_.push_back(std::move(cb));
    }

    void wake_up() {
        return waker_.wake_up();
    }

private:
    void wait_before() {
        decltype(waiting_coros_)::value_type cb{nullptr};
        waker_.reg();

        io_uring_sqe *sqe{nullptr};
        while (!waiting_coros_.empty()) {
            sqe = ring_.get_sqe();
            if (sqe == nullptr) {
                break;
            }
            cb = std::move(waiting_coros_.front());
            waiting_coros_.pop_front();
        }
        ring_.force_submit();
    }

private:
    IORing                                            ring_;
    Waker                                             waker_{};
    time::detail::Timer                               timer_{};
    std::list<std::function<void(io_uring_sqe *sqe)>> waiting_coros_{};
};

} // namespace zedio::io::detail
