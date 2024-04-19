#pragma once

#include "zedio/common/debug.hpp"
#include "zedio/runtime/config.hpp"
#include "zedio/runtime/multi_thread/idle.hpp"
#include "zedio/runtime/multi_thread/queue.hpp"
// C++
#include <latch>
#include <thread>
#include <vector>

namespace zedio::runtime::multi_thread {

class Worker;
class Shared;

static inline thread_local Shared *t_shared{nullptr};

class Shared {
    friend class Worker;

public:
    Shared(detail::Config config)
        : config_{config}
        , idle_{config.num_workers_}
        , shutdown_{static_cast<std::ptrdiff_t>(config.num_workers_)} {
        assert(t_shared == nullptr);
        t_shared = this;
    }

    ~Shared() {
        t_shared = nullptr;
    }

public:
    void wake_up_one();

    void wake_up_all();

    void wake_up_if_work_pending();

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

    void schedule_remote(std::coroutine_handle<> handle) {
        global_queue_.push(handle);
        wake_up_one();
    }

    void schedule_remote_batch(std::list<std::coroutine_handle<>> &&handles, std::size_t n) {
        global_queue_.push_batch(std::move(handles), n);
        wake_up_one();
    }

private:
    const detail::Config  config_;
    Idle                  idle_;
    GlobalQueue           global_queue_{};
    std::latch            shutdown_;
    std::vector<Worker *> workers_{};
};

} // namespace zedio::runtime::multi_thread
