#pragma once

#include "noncopyable.hpp"

#include <unistd.h>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>

namespace zed::util {

pid_t GetTid() noexcept {
    static thread_local pid_t t_tid = ::gettid();
    return t_tid;
}

class ThreadPool : util::Noncopyable {
public:
    using Task = std::function<void()>;
    ThreadPool(const std::size_t num) { increase(num); }

    ~ThreadPool() {
        m_running = false;
        m_not_empty.notify_all();
        for (auto& thread : m_threads) {
            thread.join();
        }
    }

    void push(const std::function<void()>& task) {
        {
            std::lock_guard lock(m_tasks_mutex);
            m_tasks.push(task);
        }
        m_not_empty.notify_one();
    }

    void increase(const std::size_t num = 1) {
        std::lock_guard lock(m_threads_mutex);
        for (std::size_t i = 0; i < num; ++i) {
            m_threads.emplace_back([this]() { loop(); });
        }
        m_activity += num;
    }

    auto size() const -> std::size_t {
        std::lock_guard lock(m_threads_mutex);
        return m_threads.size();
    }

    void wait() {
        std::unique_lock lock(m_tasks_mutex);
        m_not_activity.wait(lock, [this]() -> bool { return m_activity == 0 && m_tasks.empty(); });
    }

private:
    void loop() {
        Task task;
        while (true) {
            {
                std::unique_lock lock(m_tasks_mutex);
                --m_activity;
                if (m_activity == 0) {
                    m_not_activity.notify_one();
                }
                m_not_empty.wait(lock, [this]() -> bool { return !m_running || !m_tasks.empty(); });
                ++m_activity;
                if (!m_running) {
                    break;
                }
                task = std::move(m_tasks.front());
                m_tasks.pop();
            }
            task();
        }
    }

private:
    std::vector<std::thread> m_threads{};
    std::queue<Task>         m_tasks{};
    mutable std::mutex       m_threads_mutex{};
    mutable std::mutex       m_tasks_mutex{};
    std::condition_variable  m_not_empty{};
    std::condition_variable  m_not_activity{};
    std::atomic<std::size_t> m_activity{0};
    std::atomic<bool> m_running{true};
};

}  // namespace zed::util
