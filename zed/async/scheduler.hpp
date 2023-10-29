#pragma once

#include "async/detail/processor.hpp"
#include "async/detail/timer.hpp"
#include "async/detail/waker.hpp"

namespace zed::async {

class Scheduler : util::Noncopyable {
public:
    Scheduler(std::size_t worker_num = std::thread::hardware_concurrency())
        : pool_(&uring_, &timer_, worker_num) {
        if (auto res = io_uring_queue_init(1024 * worker_num, &uring_, IORING_SETUP_SQPOLL);
            res != 0) [[unlikely]] {
            throw std::runtime_error(std::format(
                "call io_uring_queue_init failed, errno: {}, message: {}", -res, strerror(-res)));
        }
    }

    ~Scheduler() {
        pool_.stop();
        io_uring_queue_exit(&uring_);
    }

    void start() {
        if (running_) [[unlikely]] {
            return;
        }
        running_ = true;

        pool_.submit([this]() { this->timer_.start(); });
        pool_.submit([this]() { this->waker_.start(); });
        pool_.start();
        io_uring_cqe *cqe;

        __kernel_timespec ts;
        memset(&ts, 0, sizeof(ts));
        ts.tv_sec = 10;

        while (running_) {
            io_uring_wait_cqe_timeout(&uring_, &cqe, &ts);
            if (cqe) {
                auto io_awaiter
                    = reinterpret_cast<detail::LazyBaseIOAwaiter *>(io_uring_cqe_get_data64(cqe));
                io_awaiter->res_ = cqe->res;
                auto task = [io_awaiter]() { io_awaiter->handle_.resume(); };
                pool_.submit(task);
                io_uring_cqe_seen(&uring_, cqe);
            }
        }
    }

    void stop(std::chrono::milliseconds delay = 0ms) {
        auto task = [this]() {
            this->running_ = false;
            waker_.wake();
        };
        if (delay != 0ms) {
            timer_.add_timer_event(task, delay);
        } else {
            task();
        }
    }

    void submit(Task<void> task) {
        auto handle = task.take();
        auto f = [handle]() { handle.resume(); };
        pool_.submit(f);
    }

private:
    io_uring              uring_{};
    detail::Timer         timer_{};
    detail::Waker         waker_{};
    detail::ProcessorPool pool_;
    std::atomic<bool>     running_{false}; // TODO consider delete this variable
};

} // namespace zed::async
