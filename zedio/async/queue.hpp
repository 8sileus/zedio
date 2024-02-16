#pragma once

#include "zedio/common/config.hpp"
#include "zedio/common/debug.hpp"

// C
#include <cassert>
// C++
#include <array>
#include <atomic>
#include <coroutine>
#include <expected>
#include <format>
#include <list>
#include <memory>
#include <utility>

namespace zedio::async::detail {

class GlobalQueue {
public:
    [[nodiscard]]
    auto size() -> std::size_t {
        return num_.load(std::memory_order::acquire);
    }

    [[nodiscard]]
    auto empty() -> bool {
        return num_ == 0;
    }

    void push(std::coroutine_handle<> &&task) {
        std::lock_guard lock(mutex_);
        if (is_closed_) {
            return;
        }
        tasks_.push_back(task);
        num_.fetch_add(1, std::memory_order::release);
    }

    void push(std::list<std::coroutine_handle<>> &&tasks, std::size_t n) {
        std::lock_guard lock(mutex_);
        if (is_closed_) {
            return;
        }
        tasks_.splice(tasks_.end(), tasks);
        num_.fetch_add(n, std::memory_order::seq_cst);
    }

    [[nodiscard]]
    auto pop() -> std::optional<std::coroutine_handle<>> {
        std::lock_guard lock(mutex_);
        if (tasks_.empty()) {
            return std::nullopt;
        }
        auto result = std::move(tasks_.front());
        tasks_.pop_front();
        num_.fetch_sub(1, std::memory_order::release);
        return result;
    }

    [[nodiscard]]
    auto pop_n(std::size_t &n) -> std::list<std::coroutine_handle<>> {
        std::lock_guard lock(mutex_);

        n = std::min(num_.load(std::memory_order::relaxed), n);
        num_.fetch_sub(n, std::memory_order::release);

        std::list<std::coroutine_handle<>> result;

        auto end = tasks_.begin();
        std::advance(end, n);
        result.splice(result.end(), tasks_, tasks_.begin(), end);
        return result;
    }

    [[nodiscard]]
    auto close() -> bool {
        std::lock_guard lock{mutex_};
        if (is_closed_) {
            return false;
        }
        is_closed_ = true;
        return true;
    }

    [[nodiscard]]
    auto is_closed() -> bool {
        std::lock_guard lock{mutex_};
        return is_closed_;
    }

private:
    std::list<std::coroutine_handle<>> tasks_{};
    std::atomic<std::size_t>           num_{0};
    std::mutex                         mutex_{};
    bool                               is_closed_{false};
};

class LocalQueue {
public:
    [[nodiscard]]
    auto remaining_slots() -> std::size_t {
        auto [steal, _] = unpack(head_.load(std::memory_order::acquire));
        std::atomic_ref<uint32_t> atoimc_tail{tail_};
        auto                      tail = atoimc_tail.load(std::memory_order::acquire);
        assert(zedio::config::LOCAL_QUEUE_CAPACITY >= static_cast<std::size_t>(tail - steal));
        return zedio::config::LOCAL_QUEUE_CAPACITY - static_cast<std::size_t>(tail - steal);
    }

    [[nodiscard]]
    auto size() -> uint32_t {
        auto [_, head] = unpack(head_.load(std::memory_order::acquire));
        std::atomic_ref<uint32_t> atoimc_tail{tail_};
        auto                      tail = atoimc_tail.load(std::memory_order::acquire);
        return tail - head;
    }

    [[nodiscard]]
    auto empty() -> bool {
        return size() == 0u;
    }

    [[nodiscard]]
    constexpr auto capacity() const -> std::size_t {
        return buffer_.max_size();
    }

    // Caller must ensure that there is no overflow before calling the method
    template <class C>
    void push_batch(const C &tasks, std::size_t len) {
        assert(0 < len && len <= capacity());

        auto [steal, _] = unpack(head_.load(std::memory_order::acquire));
        auto tail = tail_;

        if (tail - steal > static_cast<uint32_t>(capacity() - len)) [[unlikely]] {
            throw std::runtime_error(std::format("push_back overflow! cur size {}, push size {}",
                                                 tail - steal, capacity() - len));
        }

        for (auto &&task : tasks) {
            std::size_t idx = tail & MASK;
            buffer_[idx] = std::move(task);
            tail += 1;
            // tail = wrapping_add(tail, 1);
        }

        std::atomic_ref<uint32_t> atomic_tail{tail_};
        atomic_tail.store(tail, std::memory_order::release);
    }

    void push_back_or_overflow(std::coroutine_handle<> &&task, GlobalQueue &global_queue) {
        // LOG_TRACE("push a task");
        uint32_t tail{0};
        while (true) {
            auto head = head_.load(std::memory_order::acquire);
            auto [steal, real] = unpack(head);
            tail = tail_;
            if (tail - steal < static_cast<uint32_t>(zedio::config::LOCAL_QUEUE_CAPACITY)) {
                // There is capacity for the task
                break;
            } else if (steal != real) {
                // Some thread is stealing from current thread now
                // so only push coro to the global queue
                global_queue.push(std::move(task));
                return;
            } else {
                // Push half the coros from current queue
                if (push_overflow(task, real, tail, global_queue)) {
                    return;
                }
            }
        }
        push_back_finish(std::move(task), tail);
    }

    [[nodiscard]]
    auto pop() -> std::optional<std::coroutine_handle<>> {
        auto        head = head_.load(std::memory_order::acquire);
        std::size_t idx = 0;
        while (true) {
            auto [steal, real] = unpack(head);
            auto tail = tail_;
            if (real == tail) {
                // Queue is empty
                return std::nullopt;
            }
            auto next_real = real + 1;
            auto next = steal == real ? pack(next_real, next_real) : pack(steal, next_real);
            if (head_.compare_exchange_strong(head, next, std::memory_order::acq_rel,
                                              std::memory_order::acquire)) {
                idx = static_cast<std::size_t>(real) & MASK;
                break;
            }
        }
        return buffer_[idx];
    }

    auto steal_into(LocalQueue &dst) -> std::optional<std::coroutine_handle<>> {
        // Consider: other thread is stealing from caller
        // Caller may looks empty but still have values contained in the buffer
        std::optional<std::coroutine_handle<>> result{std::nullopt};

        auto [steal, _] = unpack(dst.head_.load(std::memory_order::acquire));
        auto dst_tail = dst.tail_;

        // less than half of local_queue_capacity just return
        if (dst_tail - steal > static_cast<uint32_t>(zedio::config::LOCAL_QUEUE_CAPACITY / 2)) {
            return result;
        }
        auto n = steal_into2(dst, dst_tail);
        if (n == 0) {
            return result;
        }
        // LOG_TRACE("steal {} works", n);
        /// Take the final task for result
        n -= 1;
        auto        dst_new_tail = dst_tail + n;
        std::size_t idx = static_cast<std::size_t>(dst_new_tail) & MASK;
        result.emplace(std::move(dst.buffer_[idx]));
        if (n > 0) {
            std::atomic_ref atomic_dst_tail{dst.tail_};
            atomic_dst_tail.store(dst_new_tail, std::memory_order::release);
        }
        return result;
    }

private:
    auto steal_into2(LocalQueue &dst, uint32_t dst_tail) -> uint32_t {
        // t1: src_head ={src_steal, src_real}
        uint64_t prev_packed = head_.load(std::memory_order::acquire);
        uint64_t next_packed;
        uint32_t n = 0;
        while (true) {
            auto [src_steal, src_real] = unpack(prev_packed);
            std::atomic_ref<uint32_t> atomic_src_tail{tail_};
            auto                      src_tail = atomic_src_tail.load(std::memory_order::acquire);

            if (src_steal != src_real) {
                // Other thread is stealing
                return 0;
            }

            n = src_tail - src_real;
            n = n - n / 2;
            if (n == 0) {
                return 0;
            }
            auto next_src_real = src_real + n;
            assert(src_steal != next_src_real);
            next_packed = pack(src_steal, next_src_real);
            if (head_.compare_exchange_strong(prev_packed, next_packed, std::memory_order::acq_rel,
                                              std::memory_order::acquire)) {
                // t2: src_head ={src_steal, src_real + steal_num}
                break;
            }
        }
        auto [first, _] = unpack(next_packed);
        for (uint32_t i = 0; i < n; ++i) {
            auto src_idx = static_cast<std::size_t>(first + i) & MASK;
            auto dst_idx = static_cast<std::size_t>(dst_tail + i) & MASK;
            dst.buffer_[dst_idx] = std::move(buffer_[src_idx]);
        }

        prev_packed = next_packed;
        while (true) {
            auto [_, head] = unpack(prev_packed);
            next_packed = pack(head, head);
            if (head_.compare_exchange_strong(prev_packed, next_packed, std::memory_order::acq_rel,
                                              std::memory_order::acquire)) {
                // t3: src_head ={src_steal + steal_num, src_real + steal_num}
                return n;
            }
        }
    }

    void push_back_finish(std::coroutine_handle<> &&task, uint32_t tail) {
        std::size_t idx = tail & MASK;
        buffer_[idx] = std::move(task);
        std::atomic_ref atomic_tail{tail_};
        atomic_tail.store(tail + 1, std::memory_order::release);
    }

    auto push_overflow(std::coroutine_handle<> &task, uint32_t head, [[maybe_unused]] uint32_t tail,
                       GlobalQueue &global_queue) -> bool {
        static constexpr auto NUM_TASKS_TAKEN{
            static_cast<uint32_t>(zedio::config::LOCAL_QUEUE_CAPACITY / 2)};

        assert(tail - head == zedio::config::LOCAL_QUEUE_CAPACITY);

        auto prev = pack(head, head);

        if (!head_.compare_exchange_strong(
                prev, pack(head + NUM_TASKS_TAKEN, head + NUM_TASKS_TAKEN),
                std::memory_order::release, std::memory_order::relaxed)) {
            return false;
        }

        std::list<std::coroutine_handle<>> tasks;
        for (uint32_t i = 0; i < NUM_TASKS_TAKEN; ++i) {
            std::size_t idx = static_cast<std::size_t>(head + i) & MASK;
            tasks.push_back(std::move(buffer_[idx]));
        }
        tasks.push_back(std::move(task));
        global_queue.push(std::move(tasks), NUM_TASKS_TAKEN + 1);
        return true;
    }

private:
    static constexpr std::size_t MASK = zedio::config::LOCAL_QUEUE_CAPACITY - 1;

    // [[nodiscard]]
    // static auto wrapping_add(uint32_t a, uint32_t b) -> uint32_t {
    //     return a + b;
    // }

    // [[nodiscard]]
    // static auto wrapping_sub(uint32_t a, uint32_t b) -> uint32_t {
    //     return a - b;
    // }

    [[nodiscard]]
    static auto unpack(uint64_t head) -> std::pair<uint32_t, uint32_t> {
        return {static_cast<uint32_t>(head >> 32), static_cast<uint32_t>(head)};
    }

    [[nodiscard]]
    static auto pack(uint32_t steal, uint32_t real) -> uint64_t {
        return static_cast<uint64_t>(steal) << 32 | static_cast<uint64_t>(real);
    }

private:
    std::atomic<uint64_t> head_{0};
    uint32_t              tail_{0};
    // std::atomic<uint32_t>                                                  tail_{0};
    std::array<std::coroutine_handle<>, zedio::config::LOCAL_QUEUE_CAPACITY> buffer_;
};

} // namespace zedio::async::detail
