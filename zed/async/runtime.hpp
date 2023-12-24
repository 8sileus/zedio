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

void spawn(Task<void> &&task) {
    detail::t_worker->schedule_task(std::move(task.take()));
}

} // namespace zed::async
