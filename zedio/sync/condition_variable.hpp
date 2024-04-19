#pragma once

#include "zedio/async/coroutine/task.hpp"
#include "zedio/sync/mutex.hpp"

// C++
#include <mutex>

namespace zedio::sync {

class ConditionVariable {
    class Awaiter {
        friend ConditionVariable;

    public:
        Awaiter(ConditionVariable &cv, Mutex &mutex)
            : cv_{cv}
            , mutex_{mutex} {}

        auto await_ready() const noexcept -> bool {
            return false;
        }

        void await_suspend(std::coroutine_handle<> handle) noexcept {
            handle_ = handle;

            std::lock_guard<Mutex> lock{mutex_, std::adopt_lock};
            next_ = cv_.awaiters_.load(std::memory_order::relaxed);
            while (!cv_.awaiters_.compare_exchange_weak(next_,
                                                        this,
                                                        std::memory_order::acquire,
                                                        std::memory_order::relaxed)) {
            }
        }

        void await_resume() const noexcept {}

    private:
        ConditionVariable      &cv_;
        Mutex                  &mutex_;
        std::coroutine_handle<> handle_;
        Awaiter                *next_{nullptr};
    };

public:
    ConditionVariable() = default;

    // Delete copy
    ConditionVariable(const ConditionVariable &) = delete;
    auto operator=(const ConditionVariable &) -> ConditionVariable & = delete;
    // Delete move
    ConditionVariable(ConditionVariable &&) = delete;
    auto operator=(ConditionVariable &&) -> ConditionVariable & = delete;

    void notify_one() noexcept {
        auto awaiters = awaiters_.load(std::memory_order::relaxed);
        if (awaiters == nullptr) {
            return;
        }
        while (!awaiters_.compare_exchange_weak(awaiters,
                                                awaiters->next_,
                                                std::memory_order::acq_rel,
                                                std::memory_order::relaxed)) {
        }
        awaiters->next_ = nullptr;
        ConditionVariable::resume(awaiters);
    }

    void notify_all() noexcept {
        auto awaiters = awaiters_.load(std::memory_order::relaxed);
        while (!awaiters_.compare_exchange_weak(awaiters,
                                                nullptr,
                                                std::memory_order::acq_rel,
                                                std::memory_order::relaxed)) {
        }
        ConditionVariable::resume(awaiters);
    }

    template <class Predicate>
        requires std::is_invocable_r_v<bool, Predicate>
    [[REMEMBER_CO_AWAIT]]
    auto wait(Mutex &mutex, Predicate &&predicate) -> async::Task<void> {
        while (!predicate()) {
            co_await Awaiter{*this, mutex};
            co_await mutex.lock();
        }
        co_return;
    }

private:
    static void resume(Awaiter *awaiter) {
        while (awaiter != nullptr) {
            runtime::detail::schedule_local(awaiter->handle_);
            awaiter = awaiter->next_;
        }
    }

private:
    std::atomic<Awaiter *> awaiters_{nullptr};
};

} // namespace zedio::sync
