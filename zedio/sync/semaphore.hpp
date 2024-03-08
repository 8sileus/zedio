#pragma once

#include "zedio/common/macros.hpp"
#include "zedio/runtime/worker.hpp"

namespace zedio::sync {

template <std::ptrdiff_t max_count>
class CountingSemaphore {
    class Awaiter {
        friend CountingSemaphore;

    public:
        explicit Awaiter(CountingSemaphore &sem)
            : sem_{sem} {}

        auto await_ready() const noexcept -> bool {
            return sem_.count_down();
        }

        void await_suspend(std::coroutine_handle<> handle) noexcept {
            handle_ = handle;
            next_ = sem_.head_.load(std::memory_order::acquire);
            while (!sem_.head_.compare_exchange_weak(next_,
                                                     this,
                                                     std::memory_order::acq_rel,
                                                     std::memory_order::relaxed)) {
            }
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
    auto acquire() {
        return Awaiter{*this};
    }

    void release(std::ptrdiff_t update = 1) {
        while (update--) {
            if (count_up()) {
                continue;
            }
            auto head = head_.load(std::memory_order::relaxed);
            while (!head_.compare_exchange_weak(head,
                                                head->next_,
                                                std::memory_order::acq_rel,
                                                std::memory_order::relaxed)) {
            }
            runtime::detail::t_worker->schedule_task(head->handle_);
        }
    }

    [[nodiscard]]
    auto max() {
        return max_count;
    }

    [[nodiscard]]
    auto try_acquire() -> bool {
        if (count_down()) {
            return true;
        } else {
            count_up();
            return false;
        }
    }

    // // TODO
    // [[REMEMBER_CO_AWAIT]]
    // auto try_acquire_until(std::chrono::steady_clock::time_point deadline) {}

    // // TODO
    // [[REMEMBER_CO_AWAIT]]
    // auto try_acquire_for(std::chrono::nanoseconds timeout) {
    //     return try_acquire_until(std::chrono::steady_clock::now() + timeout);
    // }

private:
    [[nodiscard]]
    auto count_down() -> bool {
        auto odd = count_.fetch_sub(1, std::memory_order::release);
        return odd > 0;
    }

    [[nodiscard]]
    auto count_up() -> bool {
        auto odd = count_.fetch_add(1, std::memory_order::release);
        return odd >= 0;
    }

private:
    std::atomic<std::ptrdiff_t> count_;
    std::atomic<Awaiter *>      head_{nullptr};
};

using BinarySemaphore = CountingSemaphore<2>;

} // namespace zedio::sync
