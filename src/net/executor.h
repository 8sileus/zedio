#pragma once

#include <atomic>
#include <exception>
#include <functional>
#include <unordered_set>

#include <sys/epoll.h>
#include <sys/eventfd.h>

#include "coro/task.hpp"
#include "log/log.hpp"
#include "util/thread.hpp"

namespace zed {

class Executor {
public:
    using Func = std::function<void()>;

    explicit Executor() : m_tid{util::GetTid()} {
        if (t_executor != nullptr) [[unlikely]] {
            log::Error("more than one executor");
            throw std::runtime_error(std::format("Thread{} already has a executor", util::GetTid()))
        }
        t_executor = this;
        m_poll_fd = epoll_create1(EPOLL_CLOEXEC);
        if (m_epoll_fd < 0) {
            log::Error("epoll_create failed");
            throw std::runtime_error(std::format("epoll_create failed {}", strerror(errno)));
        }
        m_wake_fd = eventfd(0, EFD_NONBLOCK);
        if (m_wake_fd < 0) {
            log::Error("eventfd failed");
            throw std::runtime_error(std::format("eventfd failed {}", strerror(errno)));
        }
    }

    ~Executor();

    void start();
    void stop();

    void update_event(int fd, epoll_event event, bool wakeup = true);
    void delete_event(int fd, bool wakeup = true);

    void submit(Func task, bool wakeup = true);
    void submit(std::initializer_list<Func> tasks, bool wakeup = true);
    template <typename T>
    void submit(coro::Task<T>&& Task, bool wakeup = true);

    auto tid() const noexcept -> pid_t { return m_tid; }

private:
    /// @brief wakeup this executor when it is executing epoll_wait
    void wakeup();

    auto is_current_thread() const { return m_tid == util::GetTid(); }

private:
    int                     m_epoll_fd{-1};
    int                     m_wake_fd{-1};
    int                     m_timer_fd{-1};
    pid_t                   m_tid{0};
    std::atomic<bool>       m_running{false};
    std::unordered_set<int> m_fds{};

    std::mutex        m_mutex{};
    std::vector<Func> m_pending_tasks{};
};

static thread_local Executor* t_executor{nullptr};

auto GetCurrentExecutor() noexcept -> Executor* { return t_executor; }

}  // namespace zed
