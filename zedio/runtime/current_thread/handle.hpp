#pragma once

#include "zedio/runtime/current_thread/worker.hpp"

namespace zedio::runtime::current_thread {

class Handle {
public:
    Handle(detail::Config config, std::function<std::string(std::size_t)> build_thread_name_func)
        : worker_{config} {
        util::set_current_thread_name(build_thread_name_func(0));
    }

    ~Handle() {
        close();
    }

public:
    void schedule_task(std::coroutine_handle<> handle) {
        worker_.schedule_remote(handle);
    }

    void close() {
        worker_.close();
    }

    void wait() {
        worker_.run();
    }

public:
    Worker worker_;
};

static inline void schedule_local(std::coroutine_handle<> handle) {
    if (t_worker == nullptr) [[unlikely]] {
        std::unreachable();
    }
    t_worker->schedule_local(handle);
}

static inline void schedule_remote(std::coroutine_handle<> handle) {
    if (t_worker == nullptr) [[unlikely]] {
        std::unreachable();
    }
    t_worker->schedule_remote(handle);
}

static inline void schedule_remote_batch(std::list<std::coroutine_handle<>> &&handles,
                                         std::size_t                          n) {
    if (t_worker == nullptr) [[unlikely]] {
        std::unreachable();
    }
    t_worker->schedule_remote_batch(std::move(handles), n);
}

} // namespace zedio::runtime::current_thread