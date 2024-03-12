#pragma once

#include "zedio/common/error.hpp"
#include "zedio/common/static_math.hpp"
#include "zedio/time/timer/wheel.hpp"

namespace zedio::time::detail {

class Timer;

inline thread_local Timer *t_timer;

constexpr inline std::size_t MAX_LEVEL = 6;

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
        if (expiration_time <= last_handle_time_) [[unlikely]] {
            return std::unexpected{make_zedio_error(Error::PassedTime)};
        }
        if (expiration_time >= start_ + std::chrono::milliseconds{MAX_MS}) [[unlikely]] {
            return std::unexpected{make_zedio_error(Error::TooLongTime)};
        }
        auto interval
            = std::chrono::duration_cast<std::chrono::milliseconds>(expiration_time - start_)
                  .count();

        auto [entry, result] = Entry::make(expiration_time, handle);
        wheel.add_entry(std::move(entry), interval);
        return result;
    }

    void remove_entry(std::shared_ptr<Entry> &&entry) {
        auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(
                            entry->expiration_time_ - start_)
                            .count();
        wheel.remove_entry(std::move(entry), interval);
    }

    [[nodiscard]]
    auto next_expiration_time() -> std::optional<uint64_t> {
        if (wheel.empty()) {
            return std::nullopt;
        }
        return wheel.next_expiration_time()
               - std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::steady_clock::now() - start_)
                     .count();
    }

    [[nodiscard]]
    auto handle_expired_entries(runtime::detail::LocalQueue  &local_queue,
                                runtime::detail::GlobalQueue &global_queue) {
        last_handle_time_ = std::chrono::steady_clock::now();

        auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - start_)
                            .count();

        assert(interval >= 0);

        std::size_t count{0};

        wheel.handle_expired_entries(local_queue,
                                     global_queue,
                                     count,
                                     static_cast<std::size_t>(interval));
        rotate_if_needed(interval);
        return count;
    }

private:
    void rotate_if_needed(std::size_t interval) {
        auto n = interval / MS_PER_SLOT;
        if (n > 0) {
            wheel.rotate(n);
        }
        start_ += std::chrono::milliseconds{MS_PER_SLOT * n};
    }

private:
    //~ 12 days
    static constexpr std::size_t MS_PER_SLOT{util::static_pow(64uz, MAX_LEVEL - 1)};
    //~ 2 years
    static constexpr std::size_t MAX_MS{util::static_pow(64uz, MAX_LEVEL)};

private:
    std::chrono::steady_clock::time_point start_{std::chrono::steady_clock::now()};
    std::chrono::steady_clock::time_point last_handle_time_{std::chrono::steady_clock::now()};
    Wheel<MS_PER_SLOT>                    wheel;
};

} // namespace zedio::time::detail
