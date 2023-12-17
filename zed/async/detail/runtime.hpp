#pragma once

#include "async/detail/worker.hpp"

namespace zed::async {

class Runtime {
public:
    Runtime(std::size_t num_worker = std::thread::hardware_concurrency())
        : shared_{num_worker} {}

    void run() {
        shared_.workers_[0]->run();
    }

private:
    detail::Worker::Shared shared_;
};

void spawn(Task<void> &&task) {
    detail::t_worker->schedule_task(std::move(task.take()));
}

} // namespace zed::async
