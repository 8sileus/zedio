#pragma once

#include "zed/async/operations.hpp"
#include "zed/async/worker.hpp"

namespace zed::async {

class Runtime : util::Noncopyable {
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

template <typename T, typename... Ts>
    requires std::conjunction_v<std::is_same<T, Ts>...>
constexpr void spwan(T &&first_task, Ts &&...chain_tasks) {
    if constexpr (sizeof...(Ts) == 0) {
        detail::t_worker->schedule_task(std::move(first_task.take()));
    } else {
        std::vector<Task<void>> tasks;
        tasks.reserve(1 + sizeof...(Ts));
        tasks.push_back(std::move(first_task));
        (tasks.push_back(std::move(chain_tasks)), ...);
        auto task = [](std::vector<Task<void>> tasks) -> Task<void> {
            for (auto &task : tasks) {
                co_await task;
            }
            co_return;
        }(std::move(tasks));
        detail::t_worker->schedule_task(std::move(task.take()));
    }
}

auto add_timer_event(const std::function<void()> &cb, const std::chrono::nanoseconds &delay,
                     const std::chrono::nanoseconds &period = std::chrono::nanoseconds{0}) {
    return detail::t_worker->timer().add_timer_event(cb, delay, period);
}

} // namespace zed::async
