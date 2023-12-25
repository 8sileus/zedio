#pragma once

#include "async/worker.hpp"

namespace zed::async {

class Runtime {
public:
    Runtime(std::size_t num_worker = std::thread::hardware_concurrency())
        : shared_{num_worker} {}

    // void run() {
    // shared_.workers_[0]->run();
    // }

    void block_on(Task<void> &&task) {
        detail::t_worker->schedule_task(main_coro(std::move(task)).take());
        shared_.workers_[0]->run();
    }

private:
    detail::Worker::Shared shared_;

    auto main_coro(Task<void> &&main_task) -> Task<void> {
        co_await main_task;
        shared_.close();
    }
};

namespace detail {
    template <typename T, typename... F>
        requires std::is_same_v<T, Task<void>>
    void spwan_helper(std::vector<Task<void>> &tasks, T &&first_task, F &&...chain_tasks) {
        tasks.push_back(std::move(first_task));
        if constexpr (sizeof...(F) > 0) {
            spwan_helper(tasks, std::forward<F>(chain_tasks)...);
        }
    }
} // namespace detail

void spwan(std::vector<Task<void>> &&tasks) {
    auto handle = [](std::vector<Task<void>> tasks) -> Task<void> {
        LOG_DEBUG("2.remenber delete me {}", tasks.size());
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
void spwan(T &&first_task, F &&...chain_tasks) {
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

} // namespace zed::async
