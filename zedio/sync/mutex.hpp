#pragma once

#include "zedio/common/macros.hpp"
#include "zedio/runtime/worker.hpp"
// C++
#include <atomic>
#include <coroutine>
#include <mutex>
// C
#include <cassert>

namespace zedio::sync {

class Mutex {
    class Awaiter {
        friend Mutex;

    public:
        explicit Awaiter(Mutex &mutex)
            : mutex_{mutex} {}

        auto await_ready() const noexcept -> bool {
            return false;
        }

        auto await_suspend(std::coroutine_handle<> handle) noexcept -> bool {
            handle_ = handle;
            auto state = mutex_.state_.load(std::memory_order::relaxed);
            while (true) {
                if (state == mutex_.unlocking_state()) {
                    if (mutex_.state_.compare_exchange_weak(state,
                                                            mutex_.locking_state(),
                                                            std::memory_order::acquire,
                                                            std::memory_order::relaxed)) {
                        return false;
                    }
                } else {
                    next_ = state;
                    if (mutex_.state_.compare_exchange_weak(state,
                                                            this,
                                                            std::memory_order::acquire,
                                                            std::memory_order::relaxed)) {
                        break;
                    }
                }
            }
            return true;
        }

        constexpr void await_resume() const noexcept {}

    private:
        Mutex                  &mutex_;
        Awaiter                *next_;
        std::coroutine_handle<> handle_;
    };

public:
    Mutex()
        : state_{unlocking_state()} {}

    ~Mutex() {
        assert(state_ == unlocking_state() || state_ == locking_state());
        assert(fifo_awaiters_ == nullptr);
    }

    // Delete copy
    Mutex(const Mutex &) = delete;
    auto operator=(const Mutex &) -> Mutex & = delete;
    // Delete move
    Mutex(Mutex &&) = delete;
    auto operator=(Mutex &&) -> Mutex & = delete;

    [[nodiscard]]
    auto try_lock() noexcept -> bool {
        auto state = state_.load(std::memory_order::relaxed);
        return state_.compare_exchange_strong(state,
                                              locking_state(),
                                              std::memory_order::acquire,
                                              std::memory_order::relaxed);
    }

    [[REMEMBER_CO_AWAIT]]
    auto lock() noexcept {
        return Awaiter{*this};
    }

    void unlock() noexcept {
        assert(state_.load(std::memory_order::relaxed) != unlocking_state());

        // IF head_awaiter is nullptr,That means there are no ordered coroutines
        // to resume.
        if (fifo_awaiters_ == nullptr) {
            auto state = state_.load(std::memory_order::relaxed);
            // If current state == nullptr and state == unlocked state,That
            // means no coroutine were enqueued while i execute ordered
            // coroutines
            if (state == locking_state()
                && state_.compare_exchange_strong(state,
                                                  unlocking_state(),
                                                  std::memory_order::acquire,
                                                  std::memory_order::relaxed)) {
                return;
            }
            auto lifo_awaiters = state_.exchange(locking_state(), std::memory_order::acquire);
            assert(lifo_awaiters != locking_state() && lifo_awaiters != unlocking_state());
            // Reverse lists
            do {
                std::tie(fifo_awaiters_, lifo_awaiters, lifo_awaiters->next_)
                    = std::tuple{lifo_awaiters, lifo_awaiters->next_, fifo_awaiters_};
            } while (lifo_awaiters != nullptr);
        }
        assert(fifo_awaiters_ != nullptr);
        runtime::detail::t_worker->schedule_task(fifo_awaiters_->handle_);
        fifo_awaiters_ = fifo_awaiters_->next_;
    }

private:
    [[nodiscard]]
    auto locking_state() -> Awaiter * {
        return nullptr;
    }

    [[nodiscard]]
    auto unlocking_state() -> Awaiter * {
        return reinterpret_cast<Awaiter *>(this);
    }

private:
    // Locked     => nullptr;
    // Not locked => this;
    // other      => head of lifo queue
    std::atomic<Awaiter *> state_;
    Awaiter               *fifo_awaiters_{nullptr};
};

class LockGuard {
    LockGuard(Mutex &mutex)
        : mutex_{mutex} {}

public:
    ~LockGuard() {
        mutex_.unlock();
    }

private:
    Mutex &mutex_;
};

} // namespace zedio::sync
