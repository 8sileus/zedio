#pragma once

#include "common/config.hpp"
#include "common/debug.hpp"

// C
#include <cassert>
// C++
#include <array>
#include <atomic>
#include <coroutine>
#include <expected>
#include <memory>
#include <utility>

namespace zed::async::detail {

class LocalQueue {
public:
    [[nodiscard]]
    auto remaining_slots() -> std::size_t {
        auto [steal, _] = unpack(head_.load(std::memory_order::acquire));
        auto tail = tail_.load(std::memory_order::acquire);
        return zed::config::LOCAL_QUEUE_CAPACITY
               - static_cast<std::size_t>(wrapping_sub(tail, steal));
    }

    [[nodiscard]]
    auto size() -> uint32_t {
        auto [_, head] = unpack(head_.load(std::memory_order::acquire));
        auto tail = tail_.load(std::memory_order::acquire);
        return wrapping_sub(tail, head);
    }

    [[nodiscard]]
    auto empty() -> bool {
        return size() == 0;
    }

    [[nodiscard]]
    constexpr auto capacity() const -> std::size_t {
        return buffer_.max_size();
    }

    // Caller must ensure that there is no overflow before calling the method
    void push_back(std::list<std::coroutine_handle<>> &&tasks, std::size_t len) {
        assert(0 < len && len <= capacity());

        auto tail = tail_.load(std::memory_order::relaxed);

        for (auto &&task : tasks) {
            buffer_[tail] = std::move(task);
            tail = wrapping_add(tail, 1);
        }

        tail_.store(tail, std::memory_order::release);
    }

    void push_back_or_overflow(std::coroutine_handle<> &&task, GlobalQueue &global_queue) {
        uint32_t tail{0};
        while (true) {
            auto head = head_.load(std::memory_order::acquire);
            auto [steal, real] = unpack(head);
            tail = tail_.load(std::memory_order::relaxed);
            if (wrapping_sub(tail, steal)
                < static_cast<uint32_t>(zed::config::LOCAL_QUEUE_CAPACITY)) {
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
            auto tail = tail_.load(std::memory_order::relaxed);
            if (real == tail) {
                // Queue is empty
                return std::nullopt;
            }
            auto next_real = wrapping_add(real, 1);
            auto next = steal == real ? pack(next_real, next_real) : pack(steal, next_real);
            if (head_.compare_exchange_strong(head, next, std::memory_order::acq_rel,
                                              std::memory_order::acquire)) {
                idx = static_cast<std::size_t>(real);
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
        auto dst_tail = dst.tail_.load(std::memory_order::relaxed);

        // less than half of local_queue_capacity just return
        if (wrapping_sub(dst_tail, steal)
            > static_cast<uint32_t>(zed::config::LOCAL_QUEUE_CAPACITY / 2)) {
            return result;
        }
        auto n = steal_into2(dst, dst_tail);
        if (n == 0) {
            return result;
        }
        LOG_TRACE("steal {} tasks", n);
        /// Take the final task for result
        n -= 1;
        auto dst_new_tail = wrapping_add(dst_tail, n);
        result.emplace(std::move(dst.buffer_[dst_new_tail]));
        if (n > 0) {
            dst.tail_.store(dst_new_tail, std::memory_order::release);
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
            auto src_tail = tail_.load(std::memory_order::acquire);

            if (src_steal != src_real) {
                // Other thread is stealing
                return 0;
            }

            n = wrapping_sub(src_tail, src_real);
            n = n - n / 2;
            if (n == 0) {
                return 0;
            }
            auto next_src_real = wrapping_add(src_real, n);
            next_packed = pack(src_steal, next_src_real);
            if (head_.compare_exchange_strong(prev_packed, next_packed, std::memory_order::acq_rel,
                                              std::memory_order::acquire)) {
                // t2: src_head ={src_steal, src_real + steal_num}
                break;
            }
        }
        auto [first, _] = unpack(next_packed);
        for (auto i = 0; i < n; ++i) {
            auto src_idx = wrapping_add(first, i);
            auto dst_idx = wrapping_add(dst_tail, i);
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
        buffer_[tail] = std::move(task);
        tail_.store(wrapping_add(tail, 1), std::memory_order::release);
    }

    auto push_overflow(std::coroutine_handle<> &task, uint32_t head, uint32_t tail,
                       GlobalQueue &global_queue) -> bool {
        constexpr uint32_t NUM_TASKS_TAKEN{
            static_cast<uint32_t>(zed::config::LOCAL_QUEUE_CAPACITY / 2)};

        assert(wrapping_sub(tail, head) == zed::config::LOCAL_QUEUE_CAPACITY);

        auto prev = pack(head, head);

        if (!head_.compare_exchange_strong(
                prev,
                pack(wrapping_add(head, NUM_TASKS_TAKEN), wrapping_add(head, NUM_TASKS_TAKEN)),
                std::memory_order::release, std::memory_order::relaxed)) {
            return false;
        }

        std::list<std::coroutine_handle<>> tasks;
        for (std::size_t i = 0; i < NUM_TASKS_TAKEN; ++i) {
            tasks.push_back(std::move(buffer_[wrapping_add(head, i)]));
        }
        global_queue.push(std::move(tasks), NUM_TASKS_TAKEN);
        return true;
    }

private:
    [[nodiscard]]
    static auto wrapping_add(uint32_t a, uint32_t b) -> uint32_t {
        constexpr uint32_t limit{static_cast<uint32_t>(zed::config::LOCAL_QUEUE_CAPACITY)};
        return (a + b) % limit;
    }

    [[nodiscard]]
    static auto wrapping_sub(uint32_t a, uint32_t b) -> uint32_t {
        constexpr uint32_t limit{static_cast<uint32_t>(zed::config::LOCAL_QUEUE_CAPACITY)};
        return (a + limit - b) % limit;
    }

    [[nodiscard]]
    static auto unpack(uint64_t head) -> std::pair<uint32_t, uint32_t> {
        return {static_cast<uint32_t>(head >> 32), static_cast<uint32_t>(head)};
    }

    [[nodiscard]]
    static auto pack(uint32_t steal, uint32_t real) -> uint64_t {
        return static_cast<uint64_t>(steal) << 32 | static_cast<uint64_t>(real);
    }


private:
    std::atomic<uint64_t>                                                  head_{0};
    std::atomic<uint32_t>                                                  tail_{0};
    std::array<std::coroutine_handle<>, zed::config::LOCAL_QUEUE_CAPACITY> buffer_;
};

} // namespace zed::async::detail
