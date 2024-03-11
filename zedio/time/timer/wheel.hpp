#pragma once

#include "zedio/runtime/queue.hpp"
#include "zedio/time/timer/entry.hpp"
// C
#include <assert.h>
// C++
#include <array>
#include <bitset>

namespace zedio::time::detail {

static inline constexpr std::size_t SLOT_SIZE{64};

template <std::size_t MS_PER_SLOT>
    requires(MS_PER_SLOT > 0)
class Wheel {
private:
    using ChildWheel = Wheel<MS_PER_SLOT / SLOT_SIZE>;
    using ChildWheelPtr = std::unqiue_ptr<ChildWheel>;

public:
    void add_entry(std::unique_ptr<Entry> &&entry) {
        auto index = get_index(entry->ms_);
        if (!bitmap_[index]) {
            bitmap_[index] = 1;
            slots_[index] = std::make_unique<ChildWheel>();
        }
        slots_[index].add_entry(std::move(entry));
    };

    void remove_entry(Entry *entry) {
        auto index = get_index(entry->ms_);
        slots_[index].remove_entry(entry);
        if (slots_[index].empty()) {
            bitmap_[index] = 0;
            slots_[index].release();
        }
    }

    [[nodiscard]]
    auto empty() const noexcept -> bool {
        return bitmap_.none();
    }

    auto handle_expired_entry(runtime::detail::LocalQueue  &local_queue,
                              runtime::detail::GlobalQueue &global_queue,
                              std::chrono::milliseconds     remain) -> std::size_t {
        for (auto i = 0; i < slots_.size() && remain.count() < MS_PER_SLOT; i += 1) {
            if (bitmap_[i]) {

            }
        }
    }

private:
    [[nodiscard]]
    auto get_index(std::chrono::milliseconds ms) -> std::size_t {
        return (ms.count() & MASK) >> SHIFT;
    }

private:
    static constexpr std::size_t SHIFT = (MS_PER_SLOT / 64) * 6;
    static constexpr std::size_t MASK{(MS_PER_SLOT - 1) << SCHED_FIFO};

private:
    std::bitset<64>                      bitmap_{};
    std::array<ChildWheelPtr, SLOT_SIZE> slots_;
};

template <>
class Wheel<1> {
public:
    void add_entry(std::unique_ptr<Entry> entry) {
        auto index = get_index(entry->ms_);
        entry->next_ = std::move(slots_[index]);
        bitmap_[index] = 1;
        slots_[index] = std::move(entry);
    }

    void remove_entry(Entry *entry) {
        auto  index = get_index(entry->ms_);
        Entry head{.next_ = std::move(slots_[index])};
        auto  prev = &head;
        auto  cur = head.next_.get();
        while (cur != nullptr && cur != entry) {
            cur = cur->next_.get();
        }
        assert(cur);
        if (cur) [[likely]] {
            prev->next_ = std::move(cur->next_);
        }
        slots_[index] = std::move(head.next_);
        if (slots_[index] == nullptr) {
            bitmap_[index] = 0;
        }
    }

    [[nodiscard]]
    auto empty() const noexcept -> bool {
        return bitmap_.none();
    }

private:
    [[nodiscard]]
    auto get_index(std::chrono::milliseconds ms) -> std::size_t {
        return ms.count() & MASK;
    }

private:
    static constexpr std::size_t MASK{SLOT_SIZE - 1};

private:
    std::bitset<64>                               bitmap_{};
    std::array<std::unique_ptr<Entry>, SLOT_SIZE> slots_{};
};

} // namespace zedio::time::detail
