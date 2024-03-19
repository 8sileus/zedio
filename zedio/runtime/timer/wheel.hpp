#pragma once

#include "zedio/common/static_math.hpp"
#include "zedio/runtime/config.hpp"
#include "zedio/runtime/queue.hpp"
#include "zedio/runtime/timer/entry.hpp"
// C
#include <assert.h>
// C++
#include <array>
#include <bit>

namespace zedio::runtime::detail {

template <std::size_t level>
class Wheel {
private:
    using FatherWheel = Wheel<level + 1>;
    using FatherWheelPtr = std::unique_ptr<FatherWheel>;
    using ChildWheel = Wheel<level - 1>;
    using ChildWheelPtr = std::unique_ptr<ChildWheel>;

public:
    Wheel() {
        // LOG_DEBUG("build Wheel {}", level);
    }

    Wheel(ChildWheelPtr &&child)
        : bitmap_{1uz}
        , slots_{std::move(child)} {
        // LOG_DEBUG("level up from {} to {}", level - 1, level);
    }

    ~Wheel() {
        // LOG_DEBUG("desc Wheel {}", level);
    }

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
            slots_[index] = nullptr;
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
                slots_[index] = nullptr;
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

    [[nodiscard]]
    constexpr auto ms_per_slot() const noexcept -> std::size_t {
        return MS_PER_SLOT;
    }

    [[nodiscard]]
    constexpr auto max_ms() const noexcept -> std::size_t {
        return MS_PER_SLOT * SLOT_SIZE;
    }

    [[nodiscard]]
    auto level_up(std::unique_ptr<Wheel> &&me) -> FatherWheelPtr {
        return std::make_unique<FatherWheel>(std::move(me));
    }

    // TODO
    // auto level_down() -> ChildWheelPtr {
    // return std::move(slots_[0]);
    // }

private:
    [[nodiscard]]
    auto get_index(std::size_t interval) -> std::size_t {
        return (interval & MASK) >> SHIFT;
    }

private:
    static constexpr std::size_t SHIFT{level * 6uz};
    static constexpr std::size_t MASK{(SLOT_SIZE - 1uz) << SHIFT};
    static constexpr std::size_t MS_PER_SLOT{util::static_pow(SLOT_SIZE, level)};

private:
    uint64_t                             bitmap_{0};
    std::array<ChildWheelPtr, SLOT_SIZE> slots_{};
};

template <>
class Wheel<0uz> {
private:
    using FatherWheel = Wheel<1>;
    using FatherWheelPtr = std::unique_ptr<FatherWheel>;

public:
    Wheel() {
        // LOG_DEBUG("build Wheel {}", 0);
    }

    ~Wheel() {
        // LOG_DEBUG("desc Wheel {}", 0);
    }

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

    [[nodiscard]]
    constexpr auto ms_per_slot() const noexcept -> std::size_t {
        return MS_PER_SLOT;
    }

    [[nodiscard]]
    constexpr auto max_ms() const noexcept -> std::size_t {
        return MS_PER_SLOT * SLOT_SIZE;
    }

    void rotate(std::size_t start) {
        assert(start < slots_.size());
        for (auto i = start; i < slots_.size(); i += 1) {
            assert(slots_[i - start] == nullptr);
            slots_[i - start] = std::move(slots_[i]);
        }
        bitmap_ >>= start;
    }

    [[nodiscard]]
    auto level_up(std::unique_ptr<Wheel> &&me) -> FatherWheelPtr {
        return std::make_unique<FatherWheel>(std::move(me));
    }

private:
    [[nodiscard]]
    auto get_index(std::size_t interval) -> std::size_t {
        return interval & MASK;
    }

private:
    static constexpr std::size_t MASK{SLOT_SIZE - 1};
    static constexpr std::size_t MS_PER_SLOT{1};

private:
    uint64_t                                      bitmap_{0};
    std::array<std::shared_ptr<Entry>, SLOT_SIZE> slots_{};
};

} // namespace zedio::runtime::detail
