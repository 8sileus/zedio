#pragma once

#include "zedio/runtime/worker.hpp"

namespace zedio {

class Runtime : util::Noncopyable {
private:
    class Builder {
    public:
        [[nodiscard]]
        auto set_num_workers(std::size_t num_workers) -> Builder & {
            config_.num_workers_ = num_workers;
            return *this;
        }

        [[nodiscard]]
        auto set_ring_entries(std::size_t ring_entries) -> Builder & {
            config_.ring_entries_ = ring_entries;
            return *this;
        }

        [[nodiscard]]
        auto set_check_io_interval(uint32_t interval) -> Builder & {
            config_.check_io_interval_ = interval;
            return *this;
        }

        [[nodiscard]]
        auto set_check_gloabal_interval(uint32_t interval) -> Builder & {
            config_.check_global_interval_ = interval;
            return *this;
        }

        [[nodiscard]]
        auto set_deffer_submission(bool on) -> Builder & {
            config_.deffer_submit_ = on;
            return *this;
        }

        [[nodiscard]]
        auto set_io_uring_sqpoll(bool on) -> Builder & {
            if (on) {
                config_.io_uring_flags_ |= IORING_SETUP_SQPOLL;
            } else {
                config_.io_uring_flags_ &= ~IORING_SETUP_SQPOLL;
            }
            return *this;
        }

        [[nodiscard]]
        auto build() -> Runtime {
            return Runtime{std::move(config_)};
        }

    private:
        runtime::detail::Config config_;
    };

private:
    Runtime(runtime::detail::Config config)
        : shared_{config} {}

public:
    // Waiting for the task to close
    void block_on(async::Task<void> &&first_coro) {
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
    static auto options() -> Builder {
        return Builder{};
    }

    /// @brief Create runtime with default options
    [[nodiscard]]
    static auto create() -> Runtime {
        return Builder{}.build();
    }

private:
    runtime::detail::Worker::Shared shared_;
};

// template <typename... Ts>
//     requires std::conjunction_v<std::is_same<async::Task<void>, Ts>...> && (sizeof...(Ts) > 0)
static inline void spawn(async::Task<void> &&task) {
    using runtime::detail::t_worker;

    // if constexpr (sizeof...(Ts) == 1) {
    t_worker->schedule_task(std::move(task.take()));
    // } else {
    // auto task = [](Ts... tasks) -> Task<void> {
    // ((co_await tasks), ...);
    // co_return;
    // }(std::forward<Ts>(tasks)...);
    // t_worker->schedule_task(std::move(task.take()));
    // }
}

[[nodiscard]]
static inline auto add_timer_event(const std::function<void()>    &cb,
                                   const std::chrono::nanoseconds &delay,
                                   const std::chrono::nanoseconds &period
                                   = std::chrono::nanoseconds{0}) {
    return io::detail::t_timer->add_timer_event(cb, delay, period);
}

} // namespace zedio
