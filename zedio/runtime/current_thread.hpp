#pragma once

#include "zedio/common/util/rand.hpp"
#include "zedio/runtime/builder.hpp"

namespace zedio {

class CurrentThreadRuntime;

static inline thread_local CurrentThreadRuntime *t_runtime{nullptr};

class Queue {
public:
    [[nodiscard]]
    auto size() -> std::size_t {
        return num_;
    }

    [[nodiscard]]
    auto empty() -> bool {
        return num_ == 0;
    }

    void push(std::coroutine_handle<> task) {
        if (is_closed_) {
            return;
        }
        tasks_.push_back(task);
        ++num_;
    }

    [[nodiscard]]
    auto pop() -> std::optional<std::coroutine_handle<>> {
        if (tasks_.empty()) {
            return std::nullopt;
        }
        auto result = std::move(tasks_.front());
        tasks_.pop_front();
        --num_;
        return result;
    }

    void close() {
        if (!is_closed_) {
            is_closed_ = true;
        }
    }

    [[nodiscard]]
    auto is_closed() -> bool {
        return is_closed_;
    }

private:
    std::list<std::coroutine_handle<>> tasks_{};
    std::size_t                        num_{0};
    bool                               is_closed_{false};
};

class Shared {
public:
    explicit Shared(runtime::detail::Config config)
        : config_{config}
        , shutdown_{false} {}

    ~Shared() {
        this->close();
    }

public:
    [[nodiscard]]
    auto next_task() -> std::optional<std::coroutine_handle<>> {
        if (queue_.empty()) {
            return std::nullopt;
        }
        return queue_.pop();
    }

    void close() {
        queue_.close();
    }

    runtime::detail::Config config_;
    Queue                   queue_;
    bool                    shutdown_;
};

class CurrentThreadRuntime : util::Noncopyable {
    friend class runtime::detail::Builder<CurrentThreadRuntime>;

private:
    explicit CurrentThreadRuntime(runtime::detail::Config config)
        : shared_{config}
        , driver_{shared_.config_} {
        assert(t_runtime == nullptr);
        t_runtime = this;
    }

public:
    ~CurrentThreadRuntime() {
        t_runtime = nullptr;
    }

public:
    auto block_on(async::Task<void> &&first_coro) -> int {
        auto main_coro = [](Shared &shared, async::Task<void> &&main_coro) -> async::Task<void> {
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

        schedule_task(main_coro.take());
        run();
        return 0;
    }

    [[nodiscard]]
    static auto create() -> CurrentThreadRuntime {
        return runtime::detail::Builder<CurrentThreadRuntime>{}.build();
    }

private:
    void run() {
        while (!is_shutdown_) [[likely]] {
            tick();

            // Poll completed io events if needed
            maintenance();

            // step 1: take task from local queue or global queue
            if (auto task = get_next_task(); task) {
                execute_task(std::move(task.value()));
                continue;
            }

            // step 2: try to poll happend I/O events
            if (poll()) {
                continue;
            }

            // step 3: be sleeping
            sleep();
        }
        LOG_TRACE("stop");
    }

    void execute_task(std::coroutine_handle<> &&task) {
        task.resume();
    }

    [[nodiscard]]
    auto get_next_task() -> std::optional<std::coroutine_handle<>> {
        return shared_.next_task();
    }

    void tick() {
        tick_ += 1;
    }

    /// If need, nonblocking poll I/O events and check if the scheduler has been shutdown
    void maintenance() {
        if (this->tick_ % shared_.config_.check_io_interval_ == 0) {
            // Poll happend I/O events, I don't care if I/O events happen
            // Just a regular checking
            [[maybe_unused]] auto _ = poll();
            // regularly check that the scheduelr is shutdown
            check_shutdown();
        }
    }

    void check_shutdown() {
        if (!is_shutdown_) [[likely]] {
            is_shutdown_ = shared_.queue_.is_closed();
        }
    }

    // poll I/O events
    [[nodiscard]]
    auto poll() -> bool {
        return driver_.poll(shared_.queue_);
    }

    // Let current worker sleep
    void sleep() {
        check_shutdown();
        while (!is_shutdown_) {
            driver_.wait(shared_.queue_);
            LOG_TRACE("sleep, tick {}", tick_);
            check_shutdown();
            break;
        }
    }

public:
    void schedule_task(std::coroutine_handle<> task) {
        shared_.queue_.push(task);
    }

private:
    Shared                  shared_;
    util::FastRand          rand_{};
    uint32_t                tick_{0};
    runtime::detail::Driver driver_;
    bool                    is_shutdown_{false};
};

} // namespace zedio
