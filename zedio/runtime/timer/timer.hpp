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
    auto add_entry(std::chrono::steady_clock::time_point expiration_time,
                   std::coroutine_handle<>               handle) -> Result<std::weak_ptr<Entry>> {
        if (expiration_time <= std::chrono::steady_clock::now()) [[unlikely]] {
            return std::unexpected{make_zedio_error(Error::PassedTime)};
        }
        if (expiration_time >= start_ + std::chrono::milliseconds{MAX_MS}) [[unlikely]] {
            return std::unexpected{make_zedio_error(Error::TooLongTime)};
        }
        std::size_t interval
            = std::chrono::duration_cast<std::chrono::milliseconds>(expiration_time - start_)
                  .count();

        auto [entry, result] = Entry::make(expiration_time, handle);

        std::visit(
            [this, interval, &entry]<typename T>(T &wheel) {
                if constexpr (std::is_same_v<T, std::monostate>) {
                    auto wheel = std::make_unique<Wheel<0>>();
                    this->level_up_and_add_entry(std::move(wheel), std::move(entry), interval);
                } else {
                    this->level_up_and_add_entry(std::move(wheel), std::move(entry), interval);
                }
            },
            root_wheel_);
        num_entries_ += 1;
        return result;
    }

    void remove_entry(std::shared_ptr<Entry> &&entry) {
        auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(
                            entry->expiration_time_ - start_)
                            .count();
        assert(root_wheel_.index() != 0 && num_entries_ != 0);
        std::visit(
            [this, interval, &entry]<typename T>(T &wheel) {
                if constexpr (std::is_same_v<T, std::monostate>) {
                    std::unreachable();
                    LOG_ERROR("no entries");
                } else {
                    wheel->remove_entry(std::move(entry), interval);
                }
            },
            root_wheel_);
        num_entries_ -= 1;
        if (num_entries_ == 0) {
            root_wheel_ = std::monostate{};
        }
        // TODO consider level_downs
    }

    [[nodiscard]]
    auto next_expiration_time() -> std::optional<uint64_t> {
        return std::visit(
            [this]<typename T>(T &ptr) -> std::optional<uint64_t> {
                if constexpr (std::is_same_v<T, std::monostate>) {
                    return std::nullopt;
                } else {
                    return ptr->next_expiration_time()
                           - std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::steady_clock::now() - start_)
                                 .count();
                }
            },
            root_wheel_);
    }

    [[nodiscard]]
    auto handle_expired_entries(runtime::detail::LocalQueue  &local_queue,
                                runtime::detail::GlobalQueue &global_queue) -> std::size_t {
        if (num_entries_ == 0 || root_wheel_.index() == 0) {
            return 0uz;
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
                    change_state(wheel, interval);
                }
            },
            root_wheel_);
        return count;
    }

private:
    template <typename T>
    void change_state(T &wheel, std::size_t interval) {
        if (wheel->empty()) {
            assert(num_entries_ == 0);
            root_wheel_ = std::monostate{};
            return;
        }
        // TODO consider level_down
        auto n = interval / wheel->ms_per_slot();
        if (n > 0) {
            wheel->rotate(n);
        }
        start_ += std::chrono::milliseconds{wheel->ms_per_slot() * n};
    }

    template <class T>
    void level_up_and_add_entry(T &&wheel, std::shared_ptr<Entry> &&entry, std::size_t interval) {
        if constexpr (std::is_same_v<T, std::unique_ptr<Wheel<MAX_LEVEL + 1uz>>>) {
            assert(false);
            std::unreachable();
        } else {
            if (interval >= wheel->max_ms()) {
                level_up_and_add_entry(wheel->level_up(std::move(wheel)),
                                       std::move(entry),
                                       interval);
            } else {
                wheel->add_entry(std::move(entry), interval);
                root_wheel_ = std::move(wheel);
            }
        }
    }

private:
    //~ 139 years
    static constexpr std::size_t MAX_MS{util::static_pow(SLOT_SIZE, MAX_LEVEL + 1)};

private:
    std::chrono::steady_clock::time_point      start_{std::chrono::steady_clock::now()};
    std::size_t                                num_entries_{0};
    VariantWheelBuilder<MAX_LEVEL + 1uz>::Type root_wheel_{};

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
