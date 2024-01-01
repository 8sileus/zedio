#pragma once

#include "async/operations.hpp"
#include "async/worker.hpp"

namespace zed::async {

class Runtime {
public:
    Runtime(std::size_t num_worker = std::thread::hardware_concurrency())
        : shared_{num_worker} {}

    // Never stop
    void run() {
        shared_.workers_[0]->run();
    }

    // Waiting for the task to close
    void block_on(Task<void> &&main_task) {
        detail::t_worker->schedule_task([](Runtime *runtime, Task<void> &&task) -> Task<void> {
            co_await task;
            runtime->shared_.close();
            co_return;
        }(this, std::move(main_task))
                                                                                       .take());
        run();
    }

private:
    detail::Worker::Shared shared_;
};

namespace detail {
    template <typename T, typename... F>
        requires std::is_same_v<T, Task<void>>
    constexpr void spwan_helper(std::vector<Task<void>> &tasks, T &&first_task,
                                F &&...chain_tasks) {
        tasks.push_back(std::move(first_task));
        if constexpr (sizeof...(F) > 0) {
            spwan_helper(tasks, std::forward<F>(chain_tasks)...);
        }
    }
} // namespace detail

void spwan(std::vector<Task<void>> &&tasks) {
    auto handle = [](std::vector<Task<void>> tasks) -> Task<void> {
        for (auto &task : tasks) {
            co_await task;
        }
        co_return;
    }(std::move(tasks))
                                                           .take();
    detail::t_worker->schedule_task(std::move(handle));
}

template <typename T, typename... F>
    requires std::is_same_v<T, Task<void>>
constexpr void spwan(T &&first_task, F &&...chain_tasks) {
    if constexpr (sizeof...(F) == 0) {
        detail::t_worker->schedule_task(std::move(first_task.take()));
    } else {
        std::vector<Task<void>> tasks;
        tasks.reserve(1 + sizeof...(F));
        tasks.push_back(std::move(first_task));
        detail::spwan_helper(tasks, std::forward<F>(chain_tasks)...);
        spwan(std::move(tasks));
    }
}

auto add_timer_event(const std::function<void()> &cb, const std::chrono::nanoseconds &delay,
                     const std::chrono::nanoseconds &period = std::chrono::nanoseconds{0}) {
    return detail::t_worker->timer().add_timer_event(cb, delay, period);
}

} // namespace zed::async
