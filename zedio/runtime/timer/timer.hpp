#pragma once

#include "zedio/common/error.hpp"
#include "zedio/common/static_math.hpp"
#include "zedio/runtime/timer/util.hpp"
#include "zedio/runtime/timer/wheel.hpp"

// C++
#include <variant>

namespace zedio::runtime::detail {

class Timer;

inline thread_local Timer *t_timer;

class Timer {
public:
    Timer() {
        assert(t_timer == nullptr);
        t_timer = this;
    }

    // Delete copy
    Timer(const Timer &) = delete;
    auto operator=(const Timer &) -> Timer & = delete;
    // Delete move
    Timer(Timer &&) = delete;
    auto operator=(Timer &&) -> Timer & = delete;

public:
    template <class HandleType>
        requires std::is_invocable_v<decltype(Entry::make<HandleType>),
                                     std::chrono::steady_clock::time_point,
                                     HandleType>
    auto add_entry(std::chrono::steady_clock::time_point expiration_time, HandleType handle)
        -> Result<Entry *> {
        auto now = std::chrono::steady_clock::now();
        if (expiration_time <= now) [[unlikely]] {
            return std::unexpected{make_error(Error::PassedTime)};
        }
        if (expiration_time >= start_ + std::chrono::milliseconds{MAX_MS}) [[unlikely]] {
            return std::unexpected{make_error(Error::TooLongTime)};
        }

        auto [entry, result] = Entry::make(expiration_time, handle);

        std::visit(
            [this, now, &entry]<typename T>(T &wheel) {
                if constexpr (std::is_same_v<T, std::monostate>) {
                    start_ = now;
                    // build main wheel
                    build_wheel_and_add_entry(std::move(entry),
                                              time_since_start(entry->expiration_time_));
                } else {
                    // level up
                    level_up_and_add_entry(std::move(wheel),
                                           std::move(entry),
                                           time_since_start(entry->expiration_time_));
                }
            },
            root_wheel_);
        num_entries_ += 1;
        return result;
    }

    void remove_entry(Entry *entry) {
        assert(root_wheel_.index() != 0 && num_entries_ != 0);
        std::visit(
            [this, entry]<typename T>(T &wheel) {
                if constexpr (std::is_same_v<T, std::monostate>) {
                    std::unreachable();
                    LOG_ERROR("no entries");
                } else {
                    wheel->remove_entry(entry, time_since_start(entry->expiration_time_));
                    num_entries_ -= 1;
                    if (num_entries_ == 0) {
                        root_wheel_ = std::monostate{};
                    }
                    //  maybe optimize
                    //  else {
                    //  level_down_if_needed(std::move(wheel));
                    //  }
                }
            },
            root_wheel_);
    }

    [[nodiscard]]
    auto next_expiration_time() -> std::optional<uint64_t> {
        return std::visit(
            [this]<typename T>(T &wheel) -> std::optional<uint64_t> {
                if constexpr (std::is_same_v<T, std::monostate>) {
                    return std::nullopt;
                } else {
                    auto now = std::chrono::steady_clock::now();
                    return wheel->next_expiration_time() - time_since_start(now);
                }
            },
            root_wheel_);
    }

    template <typename LocalQueue, typename GlobalQueue>
    [[nodiscard]]
    auto handle_expired_entries(LocalQueue &local_queue, GlobalQueue &global_queue) -> std::size_t {
        if (num_entries_ == 0 || root_wheel_.index() == 0) {
            return 0;
        }

        auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - start_)
                            .count();

        assert(interval >= 0);

        std::size_t count{0};

        std::visit(
            [this, &count, &local_queue, &global_queue, interval]<typename T>(T &wheel) {
                if constexpr (std::is_same_v<T, std::monostate>) {
                    std::unreachable();
                } else {
                    wheel->handle_expired_entries(local_queue,
                                                  global_queue,
                                                  count,
                                                  static_cast<std::size_t>(interval));
                    num_entries_ -= count;
                    maintenance(wheel, interval);
                }
            },
            root_wheel_);
        return count;
    }

private:
    template <std::size_t LEVEL>
    void maintenance(std::unique_ptr<Wheel<LEVEL>> &wheel, std::size_t interval) {
        if (wheel->empty()) {
            assert(num_entries_ == 0);
            root_wheel_ = std::monostate{};
            return;
        }
        auto n = interval / Wheel<LEVEL>::MS_PER_SLOT;
        if (n > 0) {
            wheel->rotate(n);
        }
        start_ += std::chrono::milliseconds{Wheel<LEVEL>::MS_PER_SLOT * n};
        level_down_if_needed(std::move(wheel));
    }

    template <std::size_t LEVEL>
    void level_up_and_add_entry(std::unique_ptr<Wheel<LEVEL>> &&wheel,
                                std::unique_ptr<Entry>        &&entry,
                                std::size_t                     interval) {
        if constexpr (LEVEL == MAX_LEVEL + 1) {
            assert(false);
            std::unreachable();
        } else {
            if (interval >= Wheel<LEVEL>::MAX_MS) {
                level_up_and_add_entry(wheel->level_up(std::move(wheel)),
                                       std::move(entry),
                                       interval);
            } else {
                wheel->add_entry(std::move(entry), interval);
                root_wheel_ = std::move(wheel);
            }
        }
    }

    template <std::size_t LEVEL = MAX_LEVEL>
    void build_wheel_and_add_entry(std::unique_ptr<Entry> &&entry, std::size_t interval) {
        if constexpr (LEVEL > 0) {
            if (!(Wheel<LEVEL>::MS_PER_SLOT <= interval && interval < Wheel<LEVEL>::MAX_MS)) {
                build_wheel_and_add_entry<LEVEL - 1>(std::move(entry), interval);
                return;
            }
        }
        auto wheel = std::make_unique<Wheel<LEVEL>>();
        wheel->add_entry(std::move(entry), interval);
        root_wheel_ = std::move(wheel);
    }

    template <std::size_t LEVEL>
    void level_down_if_needed(std::unique_ptr<Wheel<LEVEL>> &&wheel) {
        if constexpr (LEVEL > 0) {
            if (wheel->can_level_down()) {
                level_down_if_needed(wheel->level_down());
                return;
            }
        }
        root_wheel_ = std::move(wheel);
    }

    [[nodiscard]]
    auto time_since_start(std::chrono::steady_clock::time_point expiration_time) -> std::size_t {
        return std::chrono::duration_cast<std::chrono::milliseconds>(expiration_time - start_)
            .count();
    }

private:
    //~ 139 years
    static constexpr std::size_t MAX_MS{util::static_pow(SLOT_SIZE, MAX_LEVEL + 1)};

private:
    std::chrono::steady_clock::time_point      start_{std::chrono::steady_clock::now()};
    std::size_t                                num_entries_{0};
    VariantWheelBuilder<MAX_LEVEL + 1>::Type root_wheel_{};

    // std::variant<std::monostate,
    //              std::unique_ptr<Wheel<0>>,
    //              std::unique_ptr<Wheel<1>>,
    //              std::unique_ptr<Wheel<2>>,
    //              std::unique_ptr<Wheel<3>>,
    //              std::unique_ptr<Wheel<4>>,
    //              std::unique_ptr<Wheel<5>>,
    //              std::unique_ptr<Wheel<6>>>
    //     root_wheel_{};
};

} // namespace zedio::runtime::detail
