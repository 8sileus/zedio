#pragma once

#include "async/detail/io_awaiter.hpp"
#include "async/detail/timer.hpp"
//  C++
#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
namespace zed::async::detail {

class ProcessorPool;
class Processor;

thread_local Processor *t_processor{nullptr};

class Processor {
public:
    Processor(ProcessorPool *pool, io_uring *uring, detail::Timer *timer)
        : pool_(pool)
        , uring_(uring)
        , timer_(timer) {
        assert(t_processor == nullptr);
        t_processor = this;
    }

    ~Processor() { t_processor = nullptr; }

    void start();

    auto uring() -> io_uring * { return uring_; }

    auto timer() -> Timer * { return timer_; }

    void push_awaiter(BaseIOAwaiter *awaiter) { waiting_awaiters_.push(awaiter); }

private:
    ProcessorPool              *pool_{nullptr};
    io_uring                   *uring_{nullptr};
    Timer                      *timer_{nullptr};
    std::queue<BaseIOAwaiter *> waiting_awaiters_{};
};

class ProcessorPool {
    friend class Processor;

public:
    ProcessorPool(io_uring *uring, detail::Timer *timer, std::size_t thread_num)
        : uring_(uring)
        , timer_(timer)
        , thread_num_(thread_num) {}

    ~ProcessorPool() {
        if (running_) {
            stop();
        }
        for (auto &thread : threads_) {
            thread.join();
        }
    }

    void start() {
        running_ = true;
        for (std::size_t i = 0; i < thread_num_; ++i) {
            threads_.emplace_back([this]() {
                Processor p(this, this->uring_, this->timer_);
                p.start();
            });
        }
    }

    void set_thread_num(std::size_t thread_num) { thread_num_ = thread_num; }

    void stop() {
        running_ = false;
        cond_.notify_all();
    }

    template <typename F, typename... Args>
        requires std::same_as<std::invoke_result_t<F, Args...>, void>
    void submit(F &&f, Args &&...args) {
        auto task = [f = std::forward<F>(f), ... args = std::forward<Args>(args)] { f(args...); };
        {
            std::lock_guard lock(mutex_);
            tasks_.push(std::move(task));
        }
        cond_.notify_one();
    }

private:
    io_uring                         *uring_{nullptr};
    Timer                            *timer_{nullptr};
    std::queue<std::function<void()>> tasks_{};
    std::mutex                        mutex_{};
    std::condition_variable           cond_{};
    std::vector<std::thread>          threads_{};
    std::size_t                       thread_num_{0};
    bool                              running_{false};
};

void Processor::start() {
    std::function<void()> task{};
    BaseIOAwaiter        *awaiter{nullptr};
    while (true) {
        {
            std::unique_lock lock(pool_->mutex_);
            pool_->cond_.wait(
                lock, [this]() { return !this->pool_->tasks_.empty() || !this->pool_->running_; });
            if (!pool_->running_ && pool_->tasks_.empty()) [[unlikely]] {
                break;
            }
            task = std::move(pool_->tasks_.front());
            pool_->tasks_.pop();
        }
        task();
        while (!waiting_awaiters_.empty()) {
            auto sqe = io_uring_get_sqe(uring_);
            if (sqe == nullptr) [[unlikely]] {
                break;
            }
            awaiter = std::move(waiting_awaiters_.front());
            waiting_awaiters_.pop();
            awaiter->cb_(sqe);
            io_uring_sqe_set_data64(sqe, reinterpret_cast<unsigned long long>(awaiter));
            io_uring_submit(uring_);
        }
    }
}

} // namespace zed::async::detail
