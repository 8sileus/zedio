#pragma once

#include "zedio/runtime/multi_thread/worker.hpp"

namespace zedio::runtime::multi_thread {

class Handle {
public:
    Handle(detail::Config config, std::function<std::string(std::size_t)> build_thread_name_func)
        : shared_{config} {

        LOG_TRACE("{}", config);
        for (std::size_t i = 0; i < config.num_workers_; ++i) {
            // Make sure all threads have started
            std::latch sync{2};
            auto       thread_name = build_thread_name_func(i);
            threads_.emplace_back([this, i, &thread_name, &sync]() {
                Worker worker{shared_, i};

                util::set_current_thread_name(thread_name);

                sync.count_down();

                worker.run();
            });
            sync.arrive_and_wait();
        }
    }

    ~Handle() {
        close();
        for (auto &thread : threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

public:
    void schedule_task(std::coroutine_handle<> handle) {
        shared_.schedule_remote(handle);
    }

    void close() {
        shared_.close();
    }

    void wait() {
        for (auto &thread : threads_) {
            thread.join();
        }
    }

public:
    Shared                   shared_;
    std::vector<std::thread> threads_{};
};

static inline void schedule_local(std::coroutine_handle<> handle) {
    if (t_worker == nullptr) [[unlikely]] {
        std::unreachable();
        return;
    }
    t_worker->schedule_local(handle);
}

static inline void schedule_remote(std::coroutine_handle<> handle) {
    if (t_shared == nullptr) [[unlikely]] {
        std::unreachable();
        return;
    }
    t_shared->schedule_remote(handle);
}

static inline void schedule_remote_batch(std::list<std::coroutine_handle<>> &&handles,
                                         std::size_t                          n) {
    if (t_shared == nullptr) [[unlikely]] {
        std::unreachable();
        return;
    }
    t_shared->schedule_remote_batch(std::move(handles), n);
}

} // namespace zedio::runtime::multi_thread
