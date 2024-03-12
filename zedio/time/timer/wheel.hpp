#pragma once

#include "zedio/common/static_math.hpp"
#include "zedio/runtime/queue.hpp"
#include "zedio/time/timer/entry.hpp"
// C
#include <assert.h>
// C++
#include <array>
#include <bit>

namespace zedio::time::detail {

static inline constexpr std::size_t SLOT_SIZE{64};

template <std::size_t MS_PER_SLOT>
    requires(MS_PER_SLOT > 0)
class Wheel {
private:
    using ChildWheel = Wheel<MS_PER_SLOT / SLOT_SIZE>;
    using ChildWheelPtr = std::unique_ptr<ChildWheel>;

public:
    void add_entry(std::shared_ptr<Entry> &&entry, std::size_t interval) {
        std::size_t index = get_index(interval);
        if (slots_[index] == nullptr) {
            bitmap_ |= 1uz << index;
            slots_[index] = std::make_unique<ChildWheel>();
        }
        slots_[index]->add_entry(std::move(entry), interval);
    };

    void remove_entry(std::shared_ptr<Entry> &&entry, std::size_t interval) {
        auto index = get_index(interval);

        assert(slots_[index] != nullptr);

        slots_[index]->remove_entry(std::move(entry), interval);
        if (slots_[index]->empty()) {
            bitmap_ &= ~(1uz << index);
            slots_[index].release();
        }
    }

    [[nodiscard]]
    auto empty() const noexcept -> bool {
        return bitmap_ == 0;
    }

    void handle_expired_entries(runtime::detail::LocalQueue  &local_queue,
                                runtime::detail::GlobalQueue &global_queue,
                                std::size_t                  &count,
                                std::size_t                   remaining_ms) {
        while (bitmap_) {
            std::size_t index = std::countr_zero(bitmap_);
            if (index * MS_PER_SLOT > remaining_ms) {
                break;
            }
            slots_[index]->handle_expired_entries(local_queue,
                                                  global_queue,
                                                  count,
                                                  remaining_ms - index * MS_PER_SLOT);
            if (slots_[index]->empty()) {
                slots_[index].release();
                bitmap_ &= ~(1uz << index);
            } else {
                break;
            }
        }
    }

    [[nodiscard]]
    auto next_expiration_time() -> std::size_t {
        assert(!empty());
        std::size_t index = std::countr_zero(bitmap_);
        return index * MS_PER_SLOT + slots_[index]->next_expiration_time();
    }

    void rotate(std::size_t start) {
        assert(start < slots_.size());
        for (auto i = start; i < slots_.size(); i += 1) {
            assert(slots_[i - start] == nullptr);
            slots_[i - start] = std::move(slots_[i]);
        }
        bitmap_ >>= start;
    }

private:
    [[nodiscard]]
    auto get_index(std::size_t interval) -> std::size_t {
        return (interval & MASK) >> SHIFT;
    }

private:
    static constexpr std::size_t SHIFT{util::static_log(MS_PER_SLOT, 64uz) * 6};
    static constexpr std::size_t MASK{(SLOT_SIZE - 1uz) << SHIFT};

private:
    uint64_t                             bitmap_{0};
    std::array<ChildWheelPtr, SLOT_SIZE> slots_{};
};

template <>
class Wheel<1> {
public:
    void add_entry(std::shared_ptr<Entry> &&entry, std::size_t interval) {
        auto index = get_index(interval);
        entry->next_ = std::move(slots_[index]);
        bitmap_ |= 1uz << index;
        slots_[index] = std::move(entry);
    }

    void remove_entry(std::shared_ptr<Entry> &&entry, std::size_t interval) {
        auto index = get_index(interval);

        auto head = slots_[index].get();
        if (head == entry.get()) {
            slots_[index] = std::move(head->next_);
        } else {
            auto cur = head->next_.get();
            // Do not need to check cur != nullptr
            while (cur != entry.get()) {
                head = cur;
                cur = head->next_.get();
            }
            assert(cur != nullptr);
            head->next_ = std::move(cur->next_);
        }
        if (slots_[index] == nullptr) {
            bitmap_ &= ~(1uz << index);
        }
    }

    void handle_expired_entries(runtime::detail::LocalQueue  &local_queue,
                                runtime::detail::GlobalQueue &global_queue,
                                std::size_t                  &count,
                                std::size_t                   remaining_ms) {
        while (bitmap_) {
            std::size_t index = std::countr_zero(bitmap_);
            if (index > remaining_ms) {
                break;
            }
            while (slots_[index] != nullptr) {
                slots_[index]->execute(local_queue, global_queue);
                slots_[index] = std::move(slots_[index]->next_);
                count += 1;
            }
            bitmap_ &= ~(1uz << index);
        }
    }

    [[nodiscard]]
    auto next_expiration_time() -> std::size_t {
        assert(!empty());
        return std::countr_zero(bitmap_);
    }

    [[nodiscard]]
    auto empty() const noexcept -> bool {
        return bitmap_ == 0;
    }

private:
    [[nodiscard]]
    auto get_index(std::size_t interval) -> std::size_t {
        return interval & MASK;
    }

private:
    static constexpr std::size_t MASK{SLOT_SIZE - 1};

private:
    uint64_t                                      bitmap_{0};
    std::array<std::shared_ptr<Entry>, SLOT_SIZE> slots_{};
};

} // namespace zedio::time::detail
