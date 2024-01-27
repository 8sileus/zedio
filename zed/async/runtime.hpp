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

template <typename... Ts>
    requires std::conjunction_v<std::is_same<Task<void>, Ts>...> && (sizeof...(Ts) > 0)
static inline void spwan(Ts &&...tasks) {
    if constexpr (sizeof...(Ts) == 1) {
        (detail::t_worker->schedule_task(std::move(tasks.take())), ...);
    } else {
        auto task = [](Ts... tasks) -> Task<void> {
            ((co_await tasks), ...);
            co_return;
        }(std::forward<Ts>(tasks)...);
        detail::t_worker->schedule_task(std::move(task.take()));
    }
}

template <typename... Ts>
    requires std::conjunction_v<std::is_same<Task<void>, Ts>...> && (sizeof...(Ts) > 0)
[[nodiscard]] static inline auto join(Ts &&...tasks) -> Task<void> {
    return [](Ts... tasks) -> Task<void> {
        ((co_await tasks), ...);
        co_return;
    }(std::forward<Ts>(tasks)...);
}

[[nodiscard]]
static inline auto
add_timer_event(const std::function<void()> &cb, const std::chrono::nanoseconds &delay,
                const std::chrono::nanoseconds &period = std::chrono::nanoseconds{0}) {
    return detail::t_worker->timer().add_timer_event(cb, delay, period);
}

} // namespace zed::async
