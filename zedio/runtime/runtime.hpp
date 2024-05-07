#pragma once

#include "zedio/runtime/builder.hpp"
#include "zedio/time/sleep.hpp"

namespace zedio::runtime {

template <typename Handle>
class Runtime {
public:
    template <typename... Args>
    Runtime(Args &&...args)
        : handle_{std::forward<Args>(args)...} {}

public:
    // Waiting for the task to close
    void block_on(async::Task<void> &&task) {
        auto main_coro = [](Handle &handle, async::Task<void> task) -> async::Task<void> {
            try {
                co_await task;
            } catch (const std::exception &ex) {
                LOG_ERROR("{}", ex.what());
            } catch (...) {
                LOG_ERROR("Catch a unknown exception");
            }
            handle.close();
            co_return;
        }(handle_, std::move(task));

        handle_.schedule_task(main_coro.take());

        handle_.wait();
    }

    void shutdown_timeout(std::chrono::nanoseconds delay) {
        auto task = [](Handle &handle, std::chrono::nanoseconds delay) -> async::Task<void> {
            co_await time::sleep(delay);
            handle.clsoe();
            co_return {};
        }(handle_, delay);

        handle_.schedule_task(task.take());
    }

private:
    Handle handle_;
};

namespace detail {

    static inline auto is_current_thread() -> bool {
        return current_thread::t_worker != nullptr;
    }

    static inline void schedule_local(std::coroutine_handle<> handle) {
        if (is_current_thread()) {
            current_thread::schedule_local(handle);
        } else {
            multi_thread::schedule_local(handle);
        }
    }

    static inline void schedule_remote(std::coroutine_handle<> handle) {
        if (is_current_thread()) {
            current_thread::schedule_remote(handle);
        } else {
            multi_thread::schedule_remote(handle);
        }
    }

    static inline void schedule_remote_batch(std::list<std::coroutine_handle<>> &&handles,
                                             std::size_t                          n) {
        if (is_current_thread()) {
            current_thread::schedule_remote_batch(std::move(handles), n);
        } else {
            multi_thread::schedule_remote_batch(std::move(handles), n);
        }
    }

} // namespace detail

} // namespace zedio::runtime

namespace zedio {

// template <typename... Ts>
//     requires std::conjunction_v<std::is_same<async::Task<void>, Ts>...> && (sizeof...(Ts) > 0)
static inline auto spawn(async::Task<void> &&task) {
    auto handle = task.take();
    runtime::detail::schedule_local(handle);
    return handle;
}

} // namespace zedio
