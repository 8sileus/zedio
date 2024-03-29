#pragma once

#include "zedio/runtime/builder.hpp"
#include "zedio/runtime/worker.hpp"

namespace zedio {

class Runtime : util::Noncopyable {
    friend class runtime::detail::Builder<Runtime>;

private:
    Runtime(runtime::detail::Config config)
        : shared_{config} {}

public:
    // Waiting for the task to close
    auto block_on(async::Task<void> &&first_coro) -> int {
        auto main_coro = [](runtime::detail::Worker::Shared &shared,
                            async::Task<void>              &&main_coro) -> async::Task<void> {
            try {
                co_await main_coro;
            } catch (const std::exception &ex) {
                LOG_ERROR("{}", ex.what());
            } catch (...) {
                LOG_ERROR("Catch a unknown exception");
            }
            shared.close();
            co_return;
        }(shared_, std::move(first_coro));

        shared_.push_global_task(main_coro.take());
        shared_.wake_up_one();

        wait_workers();
        return 0;
    }

private:
    void wait_workers() {
        for (auto &thread : shared_.threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

public:
    /// @brief Create runtime with custom options
    /// @example Runtime::options.set_num_worker(4).build();
    [[nodiscard]]
    static auto options() -> runtime::detail::Builder<Runtime> {
        return runtime::detail::Builder<Runtime>{};
    }

    /// @brief Create runtime with default options
    [[nodiscard]]
    static auto create() -> Runtime {
        return runtime::detail::Builder<Runtime>{}.build();
    }

private:
    runtime::detail::Worker::Shared shared_;
};

// template <typename... Ts>
//     requires std::conjunction_v<std::is_same<async::Task<void>, Ts>...> && (sizeof...(Ts) > 0)
static inline auto spawn(async::Task<void> &&task) {
    using runtime::detail::t_worker;
    auto handle = task.take();
    t_worker->schedule_task(handle);
    return handle;
}

} // namespace zedio
