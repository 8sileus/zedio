#pragma once

#include "async/detail/timer.hpp"
#include "util/thread.hpp"
// C
#include <cassert>
// C++
#include <atomic>
#include <condition_variable>
#include <coroutine>
#include <format>
#include <functional>
#include <mutex>
#include <queue>

namespace zed::async::detail {

class Processor;

thread_local detail::Processor *t_processor{nullptr};

class Processor {
public:
    Processor(io_uring *uring)
        : tid_(this_thread::get_tid())
        , uring_(uring) {
        assert(t_processor == nullptr);
        t_processor = this;
        timer_.run();
    }

    ~Processor() { t_processor = nullptr; }

    void run() {
        assert(!running_);
        running_ = true;

        std::function<void()> task;
        BaseIOAwaiter        *awaiter{nullptr};
        while (running_) {
            {
                std::unique_lock lock(mutex_);
                cond_.wait(lock, [this]() -> bool { return !this->tasks_.empty(); });
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();

            while (!waiting_awaiters_.empty()) {
                auto sqe = io_uring_get_sqe(uring_);
                if (sqe == nullptr) [[unlikely]] {
                    break;
                }
                awaiter = waiting_awaiters_.front();
                waiting_awaiters_.pop();
                awaiter->cb_(sqe);
                io_uring_sqe_set_data64(sqe, reinterpret_cast<unsigned long long>(awaiter));
                io_uring_submit(uring_);
            }
        }
    }

    void stop() { running_ = false; }

    void push(const std::function<void()> &task) {
        {
            std::unique_lock lock(mutex_);
            tasks_.push(task);
        }
        cond_.notify_one();
    }

    void push(BaseIOAwaiter *awaiter) {
        waiting_awaiters_.push(awaiter);
        log::zed.debug("increase an awaiter,total {} waiting awaiter", waiting_awaiters_.size());
    }

    auto tid() const noexcept -> pid_t { return tid_; }

    auto uring() noexcept -> io_uring * { return uring_; }

    auto timer() -> Timer & { return timer_; }

private:
    Timer                             timer_{};
    std::queue<std::function<void()>> tasks_{};
    std::mutex                        mutex_{};
    std::condition_variable           cond_{};
    std::queue<BaseIOAwaiter *>       waiting_awaiters_{};
    pid_t                             tid_;
    io_uring                         *uring_;
    bool                              running_{false};
};

} // namespace zed::async::detail
