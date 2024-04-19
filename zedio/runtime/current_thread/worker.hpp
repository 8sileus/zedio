#pragma once

#include "zedio/common/debug.hpp"
#include "zedio/common/util/thread.hpp"
#include "zedio/runtime/config.hpp"
#include "zedio/runtime/current_thread/queue.hpp"
#include "zedio/runtime/driver.hpp"
// C++
#include <vector>

namespace zedio::runtime::current_thread {

class Worker;

static inline thread_local Worker *t_worker{nullptr};

class Worker {
public:
    Worker(const detail::Config config)
        : config_{config}
        , driver_{config_} {
        assert(t_worker == nullptr);
        t_worker = this;
    }

public:
    void run() {
        while (!is_shutdown_) [[likely]] {
            tick();

            maintenance();

            if (auto task = get_next_task(); task) {
                execute_task(std::move(task.value()));
                continue;
            }

            if (poll()) {
                continue;
            }

            sleep();
        }
        LOG_TRACE("stop");
    }

    void wake_up() {
        driver_.wake_up();
    }

    void close() {
        if (global_queue_.close()) {
            wake_up();
        }
    }

    void schedule_local(std::coroutine_handle<> task) {
        if (run_next_.has_value()) {
            local_queue_.push(run_next_.value());
        }
        run_next_.emplace(std::move(task));
    }

    void schedule_remote(std::coroutine_handle<> task) {
        global_queue_.push(task);
        wake_up();
    }

    void schedule_remote_batch(std::list<std::coroutine_handle<>> &&handles,
                               [[maybe_unused]] std::size_t         n) {
        global_queue_.push_batch(std::move(handles));
        wake_up();
    }

private:
    void execute_task(std::coroutine_handle<> task) {
        task.resume();
    }

    void tick() {
        tick_ += 1;
    }

    /// If need, nonblocking poll I/O events and check if the scheduler has been shutdown
    void maintenance() {
        if (this->tick_ % config_.io_interval_ == 0) {
            [[maybe_unused]] auto _ = poll();
            check_shutdown();
        }
    }

    void check_shutdown() {
        if (!is_shutdown_) [[likely]] {
            is_shutdown_ = global_queue_.is_closed();
        }
    }

    [[nodiscard]]
    auto get_next_task() -> std::optional<std::coroutine_handle<>> {
        if (tick_ % config_.global_queue_interval_ == 0) {
            return global_queue_.pop().or_else([this] { return next_local_task(); });
        } else {
            if (auto task = next_local_task(); task) {
                return task;
            }

            auto handles = global_queue_.pop_all();

            if (handles.empty()) {
                return std::nullopt;
            }
            auto result = std::move(handles.front());
            handles.pop_front();

            local_queue_.push_batch(handles);
            return result;
        }
    }

    [[nodiscard]]
    auto next_local_task() -> std::optional<std::coroutine_handle<>> {
        if (run_next_.has_value()) {
            std::optional<std::coroutine_handle<>> result{std::nullopt};
            result.swap(run_next_);
            assert(!run_next_.has_value());
            return result;
        }
        return local_queue_.pop();
    }

    // poll I/O events
    [[nodiscard]]
    auto poll() -> bool {
        if (!driver_.poll(local_queue_, global_queue_)) {
            return false;
        }
        return true;
    }

    // Let current worker sleep
    void sleep() {
        check_shutdown();
        while (!is_shutdown_) [[likely]] {
            LOG_TRACE("sleep, tick {}", tick_);
            driver_.wait(local_queue_, global_queue_);
            check_shutdown();
            if (!local_queue_.empty() || !global_queue_.empty()) {
                break;
            }
        }
    }

private:
    const detail::Config                   config_;
    detail::Driver                         driver_;
    GlobalQueue                            global_queue_{};
    LocalQueue                             local_queue_{};
    std::optional<std::coroutine_handle<>> run_next_{std::nullopt};
    uint32_t                               tick_{0};
    bool                                   is_shutdown_{false};
};

} // namespace zedio::runtime::current_thread
