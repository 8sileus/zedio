#pragma once

#include "zedio/common/debug.hpp"
#include "zedio/common/util/rand.hpp"
#include "zedio/common/util/thread.hpp"
#include "zedio/runtime/driver.hpp"
#include "zedio/runtime/shared.hpp"

namespace zedio::runtime::detail {

class Worker;

static inline thread_local Worker *t_worker{nullptr};

class WrappedQueue {
public:
    explicit WrappedQueue(runtime::detail::GlobalQueue &global_queue)
        : global_queue_{global_queue} {}

public:
    void push(std::coroutine_handle<> handle) {
        local_queue_.push_back_or_overflow(handle, global_queue_);
    }

    auto pop() -> std::optional<std::coroutine_handle<>> {
        return local_queue_.pop();
    }

    auto empty() -> bool {
        return local_queue_.empty();
    }

    auto size() -> std::size_t {
        return local_queue_.size();
    }

    auto remaining_slots() -> std::size_t {
        return local_queue_.remaining_slots();
    }

    auto capacity() -> std::size_t {
        return local_queue_.capacity();
    }

    auto steal_into(WrappedQueue &another) -> std::optional<std::coroutine_handle<>> {
        return local_queue_.steal_into(another.local_queue_);
    }

    template <class C>
    void push_batch(const C &tasks, std::size_t len) {
        return local_queue_.push_batch(tasks, len);
    }

private:
    GlobalQueue &global_queue_;
    LocalQueue   local_queue_;
};

class Worker : util::Noncopyable {
    friend class Shared;

public:
    Worker(Shared &shared, std::size_t index)
        : shared_{shared}
        , index_{index}
        , driver_{shared.config_}
        , queue_{shared.global_queue_} {
        current_thread::set_thread_name("ZEDIO_WORKER_" + std::to_string(index));
        LOG_TRACE("Build {}", current_thread::get_thread_name());
        assert(t_worker == nullptr);
        t_worker = this;
    }

    ~Worker() {
        t_worker = nullptr;
    }

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

            // step 2: try to steal work from other worker
            if (auto task = steal_work(); task) {
                execute_task(std::move(task.value()));
                continue;
            }

            // step 3: try to poll happend I/O events
            if (poll()) {
                continue;
            }

            // step 4: be sleeping
            sleep();
        }
        LOG_TRACE("stop");
    }

    void wake_up() {
        driver_.wake_up();
    }

    void schedule_task(std::coroutine_handle<> task) {
        if (run_next_.has_value()) {
            queue_.push(run_next_.value());
            run_next_.emplace(std::move(task));
            shared_.wake_up_one();
        } else {
            run_next_.emplace(std::move(task));
        }
    }

private:
    void execute_task(std::coroutine_handle<> &&task) {
        this->transition_from_searching();
        task.resume();
    }

    [[nodiscard]]
    auto transition_to_sleeping() -> bool {
        if (has_tasks()) {
            return false;
        }

        auto is_last_searcher = shared_.idle_.transition_worker_to_sleeping(index_, is_searching_);
        // LOG_TRACE("is last searching? {}", is_last_searcher);

        is_searching_ = false;

        if (is_last_searcher) {
            shared_.wake_up_if_work_pending();
        }
        return true;
    }

    /// Returns `true` if the transition happened.
    [[nodiscard]]
    auto transition_from_sleeping() -> bool {
        if (has_tasks()) {
            // When a worker wakes, it should only transition to the "searching"
            // state when the wake originates from another worker *or* a new task
            // is pushed. We do *not* want the worker to transition to "searching"
            // when it wakes when the I/O driver receives new events.
            is_searching_ = !shared_.idle_.remove(index_);
            return true;
        }

        if (shared_.idle_.contains(index_)) {
            return false;
        }

        is_searching_ = true;
        return true;
    }

    auto transition_to_searching() -> bool {
        if (!is_searching_) {
            is_searching_ = shared_.idle_.transition_worker_to_searching();
        }
        return is_searching_;
    }

    void transition_from_searching() {
        if (!is_searching_) {
            return;
        }
        is_searching_ = false;
        // Wake up a sleeping worker, if need
        if (shared_.idle_.transition_worker_from_searching()) {
            shared_.wake_up_one();
        }
    }

    auto has_tasks() -> bool {
        return run_next_.has_value() || !queue_.empty();
    }

    auto should_notify_others() -> bool {
        if (is_searching_) {
            return false;
        }
        // return (static_cast<std::size_t>(run_next_.has_value()) + local_queue_.size()) > 1;
        return queue_.size() > 1;
    }

    void tick() {
        tick_ += 1;
    }

    [[nodiscard]]
    auto steal_work() -> std::optional<std::coroutine_handle<>> {
        // Avoid to many worker stealing at same time
        if (!transition_to_searching()) {
            return std::nullopt;
        }
        auto num = shared_.workers_.size();
        // auto start = rand() % num;
        auto start = static_cast<std::size_t>(rand_.fastrand_n(static_cast<uint32_t>(num)));
        for (std::size_t i = 0; i < num; ++i) {
            auto idx = (start + i) % num;
            if (idx == index_) {
                continue;
            }
            if (auto result = shared_.workers_[idx]->queue_.steal_into(queue_); result) {
                return result;
            }
        }
        // Final check the global queue again
        return shared_.next_global_task();
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
            is_shutdown_ = shared_.global_queue_.is_closed();
        }
    }

    [[nodiscard]]
    auto get_next_task() -> std::optional<std::coroutine_handle<>> {
        if (tick_ % shared_.config_.check_global_interval_ == 0) {
            return shared_.next_global_task().or_else([this] { return next_local_task(); });
        } else {
            if (auto task = next_local_task(); task) {
                return task;
            }

            if (shared_.global_queue_.empty()) {
                return std::nullopt;
            }

            auto n = std::min(queue_.remaining_slots(), queue_.capacity() / 2);
            if (n == 0) [[unlikely]] {
                // All tasks of current worker are being stolen
                return shared_.next_global_task();
            }
            // We need pull 1/num_workers part of tasks
            n = std::min(shared_.global_queue_.size() / shared_.workers_.size() + 1, n);
            // n will be set to the number of tasks that are actually fetched
            auto tasks = shared_.global_queue_.pop_n(n);
            if (n == 0) {
                return std::nullopt;
            }
            auto result = std::move(tasks.front());
            tasks.pop_front();
            n -= 1;
            if (n > 0) {
                queue_.push_batch(tasks, n);
            }
            return result;
        }
    }

    [[nodiscard]]
    auto next_local_task() -> std::optional<std::coroutine_handle<>> {
        if (run_next_.has_value()) {
            // LOG_DEBUG("get a task from run_next");
            std::optional<std::coroutine_handle<>> result{std::nullopt};
            result.swap(run_next_);
            assert(!run_next_.has_value());
            return result;
        }
        // LOG_DEBUG("get a task from local_queue");
        return queue_.pop();
    }

    // poll I/O events
    [[nodiscard]]
    auto poll() -> bool {
        if (!driver_.poll(queue_)) {
            return false;
        }
        if (should_notify_others()) {
            shared_.wake_up_one();
        }
        return true;
    }

    // Let current worker sleep
    void sleep() {
        check_shutdown();
        if (transition_to_sleeping()) {
            while (!is_shutdown_) {
                driver_.wait(queue_);
                LOG_TRACE("sleep, tick {}", tick_);
                check_shutdown();
                if (transition_from_sleeping()) {
                    LOG_TRACE("awaken");
                    break;
                }
            }
        }
    }

private:
    Shared                                &shared_;
    std::size_t                            index_;
    util::FastRand                         rand_{};
    uint32_t                               tick_{0};
    std::optional<std::coroutine_handle<>> run_next_{std::nullopt};
    Driver                                 driver_;
    WrappedQueue                           queue_;
    bool                                   is_shutdown_{false};
    bool                                   is_searching_{false};
};

void schedule_local(std::coroutine_handle<> handle) {
    t_worker->schedule_task(handle);
}

/// implement Shared

Shared::Shared(Config config)
    : config_{config}
    , idle_{config.num_workers_}
    , shutdown_{static_cast<std::ptrdiff_t>(config.num_workers_)} {

    t_shared = this;

    LOG_TRACE("{}", config);
    for (std::size_t i = 0; i < config_.num_workers_; ++i) {
        // Make sure all threads have started
        std::latch sync{2};
        threads_.emplace_back([this, i, &sync]() {
            Worker worker{*this, i};
            workers_.emplace_back(&worker);
            assert(workers_.size() == i + 1);
            assert(workers_.back() == &worker);

            sync.count_down();
            worker.run();
            shutdown_.arrive_and_wait();
        });
        sync.arrive_and_wait();
    }
}

void Shared::wake_up_one() {
    if (auto index = idle_.worker_to_notify(); index) {
        LOG_TRACE("Wake up ZEDIO_WORKER_{}", index.value());
        workers_[index.value()]->wake_up();
    }
}

void Shared::wake_up_all() {
    for (auto &worker : workers_) {
        worker->wake_up();
    }
}

void Shared::wake_up_if_work_pending() {
    for (auto &worker : workers_) {
        if (!worker->queue_.empty()) {
            wake_up_one();
            return;
        }
    }
    if (!global_queue_.empty()) {
        wake_up_one();
    }
}

} // namespace zedio::runtime::detail
