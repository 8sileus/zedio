#pragma once

#include <atomic>
#include <exception>
#include <functional>
#include <unordered_set>

#include <liburing.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#include "coro/task.hpp"
#include "log/log.hpp"
#include "util/thread.hpp"

namespace zed {

class IOContext;

thread_local IOContext *t_io_context{nullptr};

class IOContext {
public:
    using Func = std::function<void()>;

    explicit IOContext()
        : m_tid{util::GetTid()} {
        // set thread_local variable
        if (t_io_context != nullptr) [[unlikely]] {
            throw std::runtime_error(std::format("thread {} already has a io context", m_tid));
        }
        t_io_context = this;

        m_wake_fd = ::eventfd(0, EFD_NONBLOCK);
        if (m_wake_fd < 0) [[unlikely]] {
            throw std::runtime_error(
                std::format("eventfd return {} {}", m_wake_fd, strerror(errno)));
        }
        // TODO from config
        if (int n = io_uring_queue_init(1024, &m_io_uring, 0); n != 0) [[unlikely]] {
            throw std::runtime_error(
                std::format("io_uring_queue_init return {} {}", n, strerror(errno)));
        }
        if (int n = io_uring_register_files(&m_io_uring, &m_wake_fd, 1); n != 0) [[unlikely]] {
            throw std::runtime_error(
                std::format("io_uring_register_files return {} {}", n, strerror(errno)));
        }
    }

    coro::Task<void> execute_pendding_task() {
        // to do
        char buf[8];

        while (m_running) {
            co_await async::Read(m_wake_fd, buf, sizeof(buf));

            std::vector<Func> tasks{};
            {
                std::lock_guard lock(m_pending_tasks_mutex);
                tasks.swap(m_pending_tasks);
            }
            for (auto &task : tasks) {
                task();
            }
        }
    }

    ~IOContext() { io_uring_queue_exit(&m_io_uring); }

private:
private:
    io_uring          m_io_uring;
    pid_t             m_tid{-1};
    int               m_wake_fd{-1};
    std::atomic<bool> m_running{false};

    std::mutex        m_pending_tasks_mutex{};
    std::vector<Func> m_pending_tasks{};
};

class Executor {
public:
    using Func = std::function<void()>;

    explicit Executor()
        : m_tid{util::GetTid()} {
        if (t_executor != nullptr) [[unlikely]] {
            log::Error("more than one executor");
            throw std::runtime_error(
                std::format("Thread{} already has a executor", util::GetTid()));
        }
        t_executor = this;
        m_epoll_fd = ::epoll_create1(EPOLL_CLOEXEC);
        if (m_epoll_fd < 0) [[unlikely]] {
            log::Error("epoll_create failed {}", strerror(errno));
            throw std::runtime_error(
                std::format("Thread{} epoll_create failed {}", m_tid, strerror(errno)));
        }
        m_wake_fd = ::eventfd(0, EFD_NONBLOCK);
        if (m_wake_fd < 0) [[unlikely]] {
            log::Error("eventfd failed {}", strerror(errno));
            throw std::runtime_error(
                std::format("Thread{} eventfd failed {}", m_tid, strerror(errno)));
        }

        struct epoll_event event;
        event.data.fd = m_wake_fd;
        event.events = EPOLLIN | EPOLLET;
        if (::epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_wake_fd, &event) != 0) [[unlikely]] {
            log::Error("epoll_ctl failed {}", strerror(errno));
            throw std::runtime_error(
                std::format("Thread{} epoll_ctl failed {}", m_tid, strerror(errno)));
        }

        log::Debug("executor construct");
    }

    ~Executor() {
        if (m_running) {
            stop();
        }
        // TODO FINISH TIMER
        //  m_timer.reset();
        ::close(m_wake_fd);
        ::close(m_epoll_fd);
        t_executor = nullptr;

        log::Debug("executor destruct");
    }

    void start() {

        assert(in_loop_thread());
        if (m_running) {
            log::Debug("executor is already running");
            return;
        }
        m_running = true;
        std::vector<epoll_event> r_events(128);
        while (m_running) {
            handle_pending_tasks();
            int cnt = epoll_wait(m_epoll_fd, r_events.data(), r_events.size(), 10000);
            if (cnt == r_events.size()) {
                r_events.resize(static_cast<std::size_t>(r_events.size() * 1.5));
            }
            log::Debug("epoll_wait: {} events happend", cnt);
            if (cnt == -1 && errno != EINTR) {
                log::Warn("epoll_wait failed [{}]", strerror(errno));
            } else {
                for (int i = 0; i < cnt; ++i) {
                    auto &event = r_events[i];
                    if (event.data.fd == m_wake_fd && (event.events & EPOLLIN)) {
                        uint64_t buf;
                        if (int n = ::read(m_wake_fd, &buf, sizeof(buf)); n != sizeof(buf)) {
                            log::Error("read wake_fd bytes: {}/{} msg: {}", n, sizeof(buf),
                                       strerror(errno));
                        }
                    } else {
                        FdEvent *ptr = static_cast<FdEvent *>(event.data.ptr);
                        if (ptr == nullptr) [[unlikely]] {
                            continue;
                        }
                        ptr->handle_event();
                    }
                }
            }
        }
    }

    void stop() {
        if (m_running) [[likely]] {
            m_running = false;
            wakeup();
            log::Debug("executor stop");
        }
    }

    template <EPOLL_OP op>
    void epoll_ctl(int fd, epoll_event &event, bool need_wakeup = true) {
        assert(fd >= 0);

        if (in_loop_thread()) {
            epoll_ctl_in_loop<op>(fd, event);
            return;
        }
        {
            std::lock_guard lock(m_mutex);
            auto            task = [this, fd, event]() { this->epoll_ctl_in_loop<op>(fd, event); };
            m_pending_tasks.emplace_back(task);
        }
        if (need_wakeup) {
            wakeup();
        }
    }

    void submit(Func task, bool need_wakeup = true) {
        std::lock_guard lock(m_mutex);
        m_pending_tasks.emplace_back(std::move(task));
        if (need_wakeup) {
            wakeup();
        }
    }

    template <typename T>
    void submit(coro::Task<T> &&Task, bool need_wakeup = true) {
        auto handle = Task.take();
        auto task = [handle = std::move(handle)]() { handle.resume(); };
        submit(std::move(task), neeed_wakeup);
    }

    auto tid() const noexcept -> pid_t { return m_tid; }

private:
    auto in_loop_thread() const noexcept -> bool { return m_tid == util::GetTid(); }

    void wakeup() {
        assert(m_running);
        uint64_t one = 1;
        if (int n = ::write(m_wake_fd, &one, sizeof(one)); n != sizeof(one)) [[unlikely]] {
            log::Error("write wake_fd bytes: {}/{} msg: {}", n, sizeof(one), strerror(errno));
        }
    }

    void handle_pending_tasks() {
        std::vector<Func> tasks;
        {
            std::lock_guard lock(m_mutex);
            m_pending_tasks.swap(tasks);
        }
        for (auto &task : tasks) {
            task();
        }
    }

    template <EPOLL_OP op>
    void epoll_ctl_in_loop(int fd, epoll_event &event) {
        assert(in_loop_thread());

        if (::epoll_ctl(m_epoll_fd, static_cast<int>(op), fd, &event) != 0) {
            log::Error("fd:{} epoll_ctl {} failed {}", fd, op_to_string(op), strerror(errno));
        }
    }

private:
    int               m_epoll_fd{-1};
    int               m_wake_fd{-1};
    int               m_timer_fd{-1};
    pid_t             m_tid{0};
    std::atomic<bool> m_running{false};

    std::mutex        m_mutex{};
    std::vector<Func> m_pending_tasks{};
};

static thread_local Executor *t_executor{nullptr};

auto GetCurrentExecutor() noexcept -> Executor * { return t_executor; }

} // namespace zed
