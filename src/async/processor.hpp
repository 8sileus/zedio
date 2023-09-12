#pragma once

#include "async/io_awaiter.hpp"
#include "async/timer.hpp"
#include "async/waker.hpp"
#include "util/parse_config.hpp"
#include "util/thread.hpp"

// Linux
#include <liburing.h>

// C++
#include <algorithm>
#include <functional>

namespace zed::async {

using namespace std::literals::chrono_literals;

class Processor : util::Noncopyable {
public:
    using Func = std::function<void()>;

    /// @brief load config from file
    Processor(const std::string &config_file_name = "zed.yaml") {
        // auto entries = get_static_config().get<unsigned>("iocontext.entries", 1024);
        // this->init(entries, IORING_SETUP_SQPOLL);
        auto parse_config = util::ParseConfig(config_file_name);
        auto entries = parse_config.get<unsigned>("processor.entries", 1024);
        auto waiting_time = parse_config.get<time_t>("processor.waiting_time", 10 * 1000);
        auto flags_str = parse_config.get<std::string>("processor.flags", "POLL");
        std::transform(flags_str.begin(), flags_str.end(), flags_str.begin(), tolower);
        auto flags = 0u;
        if (flags_str.find("POLL") != flags_str.npos) {
            flags |= IORING_SETUP_SQPOLL;
        }
        this->init(entries, flags, waiting_time);
    }

    /// @brief load config from param
    /// @param entries
    /// @param flags
    Processor(unsigned entries, unsigned flags = IORING_SETUP_SQPOLL,
              const std::chrono::milliseconds &wait_time = 10s) {
        this->init(entries, flags, wait_time.count());
    }

    ~Processor() {
        io_uring_queue_exit(&ring_);
        detail::t_ring = nullptr;
        detail::t_processor = nullptr;
    }

    void start() {
        assert(in_loop_thread());
        if (running_) {
            return;
        }

        // load from config
        io_uring_cqe *cqe = nullptr;
        while (running_) {
            // io_uring_wait_cqe_timeout()
            // int res = io_uring_wait_cqes(&io_uring, &cqe, , , );

            execute_pending_tasks();
        }
    }

    void stop() {
        if (in_loop_thread()) {
            running_ = true;
        } else {
            auto task = [this]() { return this->stop(); };
            submit(task);
        }
    }

    void submit(Task<void> coro) {
        auto handle = coro.take();
        auto task = [handle = std::move(handle)]() { handle.resume(); };
        submit(task);
    }

    void submit(const Func &f) {
        std::lock_guard lock(pending_tasks_mutex_);
        pending_tasks_.emplace_back(f);
    }

    auto add_event(const std::chrono::milliseconds &internal, const std::function<void()> &cb,
                   bool repeat) -> std::shared_ptr<detail::TimerEvent> {
        auto event = std::make_shared<detail::TimerEvent>(internal, cb, repeat);
        timer_.add_event(event);
        return event;
    }

    void wake() { waker_.wake(); }

    auto ring() -> io_uring * { return &ring_; }

private:
    auto in_loop_thread() const -> bool { return this_thread::get_tid() == tid_; }

    void init(unsigned entries, unsigned flags, time_t waiting_time) {
        waiting_time_ = waiting_time_;
        tid_ = this_thread::get_tid();
        if (this_thread::get_processor() != nullptr) [[unlikely]] {
            // TODO log error
            throw std::runtime_error(std::format("thread {} has two io_uring", tid_));
        }
        if (auto res = io_uring_queue_init(entries, &ring_, flags); res != 0) [[unlikely]] {
            throw std::runtime_error(std::format(
                "call io_uring_queue_init failed, errorno: {} message: {}", -res, strerror(-res)));
        }
        detail::t_ring = &ring_;
        detail::t_processor = this;

        timer_.run();
        waker_.run();

        // if (auto res = io_uring_register_files(&ring_, &wake_fd_, 1); res != 0) [[unlikely]] {
        //     // TODO log error
        //     throw std::runtime_error(std::format(
        //         "io_uring_register_file failed, error: {} message: {}", res, strerror(-res)));
        // }
    }

    void execute_pending_tasks() {
        std::vector<Func> tasks;
        {
            std::lock_guard lock(pending_tasks_mutex_);
            tasks.swap(pending_tasks_);
        }
        for (auto &task : tasks) {
            task();
        }
    }

    void excute_pending_coros() {

    }

private:
    detail::Waker     waker_{};
    detail::Timer     timer_{};
    io_uring          ring_{};
    pid_t             tid_{-1};
    time_t            waiting_time_{-1};
    std::mutex        pending_tasks_mutex_{};
    std::vector<Func> pending_tasks_{};
    bool              running_{false};
};

namespace detail {
thread_local Processor *t_processor;
} // namespace detail
} // namespace zed::async

namespace zed::this_thread {
static inline auto get_processor() noexcept -> zed::async::Processor * {
    return zed::async::detail::t_processor;
}
} // namespace zed::this_thread
