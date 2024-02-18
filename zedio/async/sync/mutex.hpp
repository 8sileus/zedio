#pragma once

#include "zedio/common/macros.hpp"
// C++
#include <atomic>
#include <coroutine>
#include <list>
// C
#include <cassert>

namespace zedio::async::sync {

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
            auto state = mutex_.state_.load(std::memory_order::acquire);
            while (true) {
                if (state == mutex_.unlocked_state()) {
                    if (mutex_.state_.compare_exchange_weak(
                            state, mutex_.locked_state(),
                            std::memory_order::acq_rel,
                            std::memory_order::acquire)) {
                        return false;
                    }
                } else {
                    next_ = static_cast<Awaiter *>(state);
                    if (mutex_.state_.compare_exchange_weak(
                            state, this, std::memory_order::acq_rel,
                            std::memory_order::acquire)) {
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
        : state_{this} {}

    // Forbid copy
    Mutex(const Mutex &) = delete;
    auto operator=(const Mutex &) -> Mutex & = delete;

    // Forbid move
    Mutex(Mutex &&) = delete;
    auto operator=(Mutex &&) -> Mutex & = delete;

    [[nodiscard]]
    auto try_lock() noexcept -> bool {
        void *odd = nullptr;
        return state_.compare_exchange_strong(odd, nullptr,
                                              std::memory_order::acquire,
                                              std::memory_order::relaxed);
    }

    [[REMEMBER_CO_AWAIT]]
    auto lock() noexcept {
        return Awaiter{*this};
    }

    [[REMEMBER_CO_AWAIT]]
    auto unlock() noexcept {
        assert(state_.load(std::memory_order::relaxed) != unlocked_state());
        auto head_awaiter = waiters_;

        // IF head_awaiter is nullptr,That means there are no ordered coroutines
        // to resume.
        if (head_awaiter == nullptr) {
            auto state = state_.load(std::memory_order::acquire);
            // If current state == nullptr and state == unlocked state,That
            // means no coroutine were enqueued while i execute ordered
            // coroutines
            if (state == locked_state()
                && state_.compare_exchange_strong(state, unlocked_state(),
                                                  std::memory_order::acq_rel,
                                                  std::memory_order::acquire)) {
                return;
            }
            auto current_waiter = static_cast<Awaiter *>(
                state_.exchange(nullptr, std::memory_order::acquire));
            assert(state != unlocked_state() && state != locked_state());
            // Reverse lists
            do {
                std::tie(head_awaiter, current_waiter, current_waiter->next_)
                    = std::tuple{current_waiter, current_waiter->next_,
                                 head_awaiter};
            } while (current_waiter != nullptr);
        }
        assert(head_awaiter != nullptr);
        waiters_ = head_awaiter->next_;
        head_awaiter->handle_.resume();
    }

private:
    auto locked_state() -> void * {
        return nullptr;
    }

    auto unlocked_state() -> void * {
        return this;
    }

private:
    // Locked     => nullptr;
    // Not locked => this;
    // other      => head of lifo queue
    std::atomic<void *> state_;
    Awaiter            *waiters_{nullptr};
};


class LockGuard{
    
};

} // namespace zedio::async::sync
