#pragma once

#include "zedio/runtime/builder.hpp"
#include "zedio/runtime/shared.hpp"
#include "zedio/runtime/worker.hpp"

namespace zedio {

class MultiThreadRuntime : util::Noncopyable {
    friend class runtime::detail::Builder<MultiThreadRuntime>;

private:
    explicit MultiThreadRuntime(runtime::detail::Config config)
        : shared_{config} {}

public:
    // Waiting for the task to close
    auto block_on(async::Task<void> &&first_coro) -> int {
        auto main_coro = [](runtime::detail::Shared &shared,
                            async::Task<void>      &&main_coro) -> async::Task<void> {
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

        runtime::detail::schedule_remote(main_coro.take());

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

private:
    runtime::detail::Shared shared_;
};

// // template <typename... Ts>
// //     requires std::conjunction_v<std::is_same<async::Task<void>, Ts>...> && (sizeof...(Ts)
// //     > 0)
// static inline auto spawn(async::Task<void> &&task) {
//     auto handle = task.take();
//     runtime::detail::schedule_local(handle);
//     return handle;
// }

} // namespace zedio
