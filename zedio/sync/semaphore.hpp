#pragma once

#include "zedio/common/macros.hpp"
#include "zedio/runtime/runtime.hpp"
#include "zedio/sync/mutex.hpp"

namespace zedio::sync {

template <std::ptrdiff_t max_count>
class CountingSemaphore {
    class Awaiter {
        friend CountingSemaphore;

    public:
        explicit Awaiter(CountingSemaphore &sem)
            : sem_{sem} {}

        auto await_ready() const noexcept -> bool {
            return false;
        }

        void await_suspend(std::coroutine_handle<> handle) noexcept {
            handle_ = handle;

            std::lock_guard<Mutex> lock{sem_.mutex_, std::adopt_lock};
            sem_.awaiters_.push_back(this);
        }

        constexpr void await_resume() const noexcept {}

    private:
        CountingSemaphore      &sem_;
        std::coroutine_handle<> handle_;
        Awaiter                *next_;
    };

public:
    CountingSemaphore(std::ptrdiff_t count = max_count)
        : count_{count} {
        assert(0 <= count && count <= max_count);
    }

    // Delete copy
    CountingSemaphore(const CountingSemaphore &) = delete;
    auto operator=(const CountingSemaphore &) -> CountingSemaphore & = delete;
    // Delete move
    CountingSemaphore(CountingSemaphore &&) = delete;
    auto operator=(CountingSemaphore &&) -> CountingSemaphore & = delete;

    [[REMEMBER_CO_AWAIT]]
    auto acquire() -> async::Task<void> {
        co_await mutex_.lock();
        count_ -= 1;
        if (count_ >= 0) {
            mutex_.unlock();
            co_return;
        }
        co_await Awaiter{*this};
    }

    [[REMEMBER_CO_AWAIT]]
    auto release(std::ptrdiff_t update = 1) -> async::Task<void> {
        co_await mutex_.lock();
        std::lock_guard lock(mutex_, std::adopt_lock);
        count_ += update;

        while (update > 0 && !awaiters_.empty()) {
            update -= 1;
            auto awaiter = awaiters_.front();
            awaiters_.pop_front();
            // runtime::detail::t_worker->schedule_task(awaiter->handle_);
            schedule(awaiter->handle_);
        }
    }

    [[nodiscard]]
    constexpr auto max() {
        return max_count;
    }

    // [[nodiscard]]
    // auto try_acquire() -> bool {
    //     return false;
    // }

    // // TODO
    // [[REMEMBER_CO_AWAIT]]
    // auto try_acquire_until(std::chrono::steady_clock::time_point deadline) {}

    // // TODO
    // [[REMEMBER_CO_AWAIT]]
    // auto try_acquire_for(std::chrono::nanoseconds timeout) {
    //     return try_acquire_until(std::chrono::steady_clock::now() + timeout);
    // }

private:
    Mutex                mutex_;
    std::ptrdiff_t       count_;
    std::list<Awaiter *> awaiters_{};
};

using BinarySemaphore = CountingSemaphore<1>;

} // namespace zedio::sync
