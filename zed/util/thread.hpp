#pragma once

#include "common/util/noncopyable.hpp"
// Linux
#include <pthread.h>
#include <unistd.h>
// C++
#include <condition_variable>
#include <future>
#include <queue>
#include <thread>
#include <vector>

namespace zed::this_thread {

auto get_tid() noexcept -> pid_t {
    static thread_local pid_t t_tid = ::gettid();
    return t_tid;
}
} // namespace zed::this_thread

namespace zed::util {
class SpinMutex : Noncopyable {
public:
    explicit SpinMutex(int pshared = 0) noexcept { pthread_spin_init(&mutex_, pshared); }
    ~SpinMutex() noexcept { pthread_spin_destroy(&mutex_); }

    void lock() noexcept { pthread_spin_lock(&mutex_); }
    void unlock() noexcept { pthread_spin_unlock(&mutex_); }

private:
    pthread_spinlock_t mutex_;
};

class ThreadPool {
public:
    ThreadPool(std::size_t thread_num = std::thread::hardware_concurrency())
        : thread_num_(thread_num) {}

    ~ThreadPool() {
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
            threads_.emplace_back(&ThreadPool::work, this);
        }
    }

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

    template <typename F, typename... Args, typename R = std::invoke_result_t<F, Args...>>
    auto submit(F &&f, Args &&...args) -> std::future<R> {
        auto task = [f = std::forward<F>(f), ... args = std::forward<Args>(args)] { f(args...); };
        auto p_task = std::make_shared<std::packaged_task<R>>(std::move(task));
        auto future = task->get_future();
        {
            std::lock_guard lock(mutex_);
            tasks_.emplace([p_task = std::move(p_task)] { (*p_task)(); });
        }
        cond_.notify_one();
        return future;
    }

private:
    void work() {
        if (init_func) {
            init_func();
        }
        std::function<void()> task;
        while (true) {
            {
                std::unique_lock lock(mutex_);
                cond_.wait(lock, [this]() { return !this->tasks_.empty() || !this->running_; });
                if (!running_ && tasks_.empty()) [[unlikely]] {
                    break;
                }
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();
        }
    }

private:
    std::queue<std::function<void()>> tasks_{};
    std::function<void()>             init_func{};
    std::mutex                        mutex_{};
    std::condition_variable           cond_{};
    std::vector<std::thread>          threads_{};
    std::size_t                       thread_num_{0};
    bool                              running_{false};
};

} // namespace zed::util
