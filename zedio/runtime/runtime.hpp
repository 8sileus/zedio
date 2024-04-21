#pragma once

#include "zedio/runtime/builder.hpp"
#include "zedio/runtime/current_thread.hpp"
#include "zedio/runtime/multi_thread.hpp"

namespace zedio {

template <typename R = MultiThreadRuntime>
class Runtime : util::Noncopyable {
    friend class runtime::detail::Builder<R>;

private:
    explicit Runtime(runtime::detail::Config config)
        : rt_{config} {}

public:
    // Waiting for the task to close
    auto block_on(async::Task<void> &&first_coro) -> int {
        return rt_.block_on(std::move(first_coro));
    }

public:
    /// @brief Create runtime with custom options
    /// @example Runtime<>::options().set_num_worker(4).build();
    [[nodiscard]]
    static auto options() -> runtime::detail::Builder<R> {
        return runtime::detail::Builder<R>{};
    }

    /// @brief Create runtime with default options
    [[nodiscard]]
    static auto create() -> R {
        return runtime::detail::Builder<R>{}.build();
    }

private:
    R rt_;
};

static inline auto spawn(async::Task<void> &&task) {
    assert(t_runtime || runtime::detail::t_worker);
    auto handle = task.take();
    if (t_runtime != nullptr) {
        t_runtime->schedule_task(handle);
    } else {
        runtime::detail::schedule_local(handle);
    }
    return handle;
}

static inline auto schedule(std::coroutine_handle<> handle) {
    assert(t_runtime || runtime::detail::t_worker);
    if (t_runtime != nullptr) {
        t_runtime->schedule_task(handle);
    } else {
        runtime::detail::schedule_local(handle);
    }
    return handle;
}

} // namespace zedio
