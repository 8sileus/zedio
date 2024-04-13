#pragma once

#include "zedio/common/debug.hpp"
#include "zedio/runtime/config.hpp"
#include "zedio/runtime/idle.hpp"
#include "zedio/runtime/queue.hpp"
// C++
#include <latch>
#include <thread>
#include <vector>

namespace zedio::runtime::detail {

class Worker;

class Shared;

static inline thread_local Shared *t_shared{nullptr};

class Shared {
public:
    Shared(Config config);
    //     : config_{config}
    //     , idle_{config.num_workers_}
    //     , shutdown_{static_cast<std::ptrdiff_t>(config.num_workers_)} {

    //     t_shared = this;

    //     LOG_TRACE("{}", config);
    //     for (std::size_t i = 0; i < config_.num_workers_; ++i) {
    //         // Make sure all threads have started
    //         std::latch sync{2};
    //         threads_.emplace_back([this, i, &sync]() {
    //             Worker worker{*this, i};
    //             workers_.emplace_back(&worker);
    //             assert(workers_.size() == i + 1);
    //             assert(workers_.back() == &worker);

    //             sync.count_down();
    //             worker.run();
    //             shutdown_.arrive_and_wait();
    //         });
    //         sync.arrive_and_wait();
    //     }
    // }

    ~Shared() {
        this->close();
        for (auto &thread : threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

public:
    void wake_up_one();
    // {
    // if (auto index = idle_.worker_to_notify(); index) {
    // LOG_TRACE("Wake up ZEDIO_WORKER_{}", index.value());
    // workers_[index.value()]->wake_up();
    // }
    // }

    void wake_up_all();
    // {
    // for (auto &worker : workers_) {
    // worker->wake_up();
    // }
    // }

    void wake_up_if_work_pending();
    // {
    //     for (auto &worker : workers_) {
    //         if (!worker->local_queue_.empty()) {
    //             wake_up_one();
    //             return;
    //         }
    //     }
    //     if (!global_queue_.empty()) {
    //         wake_up_one();
    //     }
    // }

    [[nodiscard]]
    auto next_global_task() -> std::optional<std::coroutine_handle<>> {
        if (global_queue_.empty()) {
            return std::nullopt;
        }
        return global_queue_.pop();
    }

    void close() {
        if (global_queue_.close()) {
            wake_up_all();
        }
    }

    const Config config_;
    GlobalQueue  global_queue_{};
    /// Coordinates idle workers
    Idle                     idle_;
    std::latch               shutdown_;
    std::vector<Worker *>    workers_{};
    std::vector<std::thread> threads_{};
};

void schedule_remote(std::coroutine_handle<> handle) {
    t_shared->global_queue_.push(handle);
    t_shared->wake_up_one();
}

void schedule_remote(std::list<std::coroutine_handle<>> &&tasks, std::size_t n) {
    t_shared->global_queue_.push(std::move(tasks), n);
    t_shared->wake_up_one();
}

} // namespace zedio::runtime::detail
