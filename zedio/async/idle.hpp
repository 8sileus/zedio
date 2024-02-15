#pragma once

// C++
#include <algorithm>
#include <atomic>
#include <limits>
#include <list>
#include <optional>
#include <utility>

namespace zedio::async::detail {

class IdleState {
public:
    IdleState(std::size_t num_workers)
        : state_{num_workers << WORKING_SHIFT} {}

    [[nodiscard]]
    auto num_searching(std::memory_order order) -> std::size_t {
        return state_.load(order) & SEARCHING_MASK;
    }

    [[nodiscard]]
    auto num_working(std::memory_order order) -> std::size_t {
        return (state_.load(order) & WORKING_MASK) >> WORKING_SHIFT;
    }

    [[nodiscard]]
    auto num_wokring_and_searching(std::memory_order order) -> std::pair<std::size_t, std::size_t> {
        auto state = state_.load(order);
        return {(state & WORKING_MASK) >> WORKING_SHIFT, state & SEARCHING_MASK};
    }

    void inc_num_searching(std::memory_order order) {
        state_.fetch_add(1, order);
    }

    /// Returns `true` if this is the final searching worker
    [[nodiscard]]
    auto dec_num_searching() -> bool {
        auto prev = state_.fetch_sub(1, std::memory_order::seq_cst);
        return (prev & SEARCHING_MASK) == 1;
    }

    void wake_up_one(std::size_t num_searching) {
        state_.fetch_add(num_searching | (1 << WORKING_SHIFT), std::memory_order::seq_cst);
    }

    /// Returns `true` if this is the final searching worker.
    [[nodiscard]]
    auto dec_num_working(bool is_searching) -> bool {
        auto dec = 1 << WORKING_SHIFT;
        if (is_searching) {
            dec += 1;
        }
        auto prev = state_.fetch_sub(dec, std::memory_order::seq_cst);
        return is_searching && (prev & SEARCHING_MASK) == 1;
    }

private:
    static constexpr std::size_t WORKING_SHIFT{16};
    static constexpr std::size_t SEARCHING_MASK{(1 << WORKING_SHIFT) - 1};
    static constexpr std::size_t WORKING_MASK{std::numeric_limits<std::size_t>::max()
                                              ^ SEARCHING_MASK};

    static_assert((WORKING_MASK & SEARCHING_MASK) == 0);
    static_assert(!(WORKING_MASK | SEARCHING_MASK) == 0);

private:
    /// --------------------------------
    /// | high 48 bit  | low  16 bit   |
    /// | working num  | searching num |
    /// --------------------------------
    std::atomic<std::size_t> state_;
};

class Idle {
    
public:
    Idle(std::size_t num_workers)
        : state_{num_workers}
        , num_workers_{num_workers} {}

    [[nodiscard]]
    auto worker_to_notify() -> std::optional<std::size_t> {
        if (!notify_should_wakeup()) {
            return std::nullopt;
        }

        std::lock_guard lock(mutex_);

        if (!notify_should_wakeup()) {
            return std::nullopt;
        }

        state_.wake_up_one(1);

        auto result = sleepers_.front();
        sleepers_.pop_front();
        return result;
    }

    [[nodiscard]]
    auto transition_worker_to_sleeping(std::size_t worker, bool is_searching) -> bool {
        std::lock_guard lock(mutex_);
        auto result = state_.dec_num_working(is_searching);
        sleepers_.push_back(worker);
        return result;
    }

    [[nodiscard]]
    auto transition_worker_to_searching() -> bool {
        if (2 * state_.num_searching(std::memory_order::seq_cst) >= num_workers_) {
            return false;
        }
        state_.inc_num_searching(std::memory_order::seq_cst);
        return true;
    }

    [[nodiscard]]
    auto transition_worker_from_searching() -> bool {
        return state_.dec_num_searching();
    }

    /// Returns `true` if the worker was sleeped before calling the method.
    [[nodiscard]]
    auto remove(std::size_t worker) -> bool {
        std::lock_guard lock(mutex_);
        for (auto it = sleepers_.begin(); it != sleepers_.end(); ++it) {
            if (*it == worker) {
                sleepers_.erase(it);
                state_.wake_up_one(0);
                return true;
            }
        }
        return false;
    }

    [[nodiscard]]
    auto contains(std::size_t worker) -> bool {
        std::lock_guard lock(mutex_);
        return std::find(sleepers_.begin(), sleepers_.end(), worker) != sleepers_.end();
    }

private:
    [[nodiscard]]
    auto notify_should_wakeup() -> bool {
        auto [num_working, num_searching]
            = state_.num_wokring_and_searching(std::memory_order::seq_cst);
        return num_searching == 0 && num_working < num_workers_;
    }

private:
    IdleState               state_;
    std::size_t             num_workers_;
    std::list<std::size_t>  sleepers_;
    std::mutex              mutex_;
};

} // namespace zedio::async::detail
