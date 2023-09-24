#pragma once

#include "async/async_io.hpp"
#include "async/task.hpp"
#include "log.hpp"
#include "util/noncopyable.hpp"
#include "util/time.hpp"
// Linux
#include <sys/timerfd.h>
// C
#include <cassert>
#include <cstring>
// C++
#include <atomic>
#include <format>
#include <functional>
#include <memory>
#include <set>
#include <shared_mutex>

using namespace std::literals::chrono_literals;

namespace zed::async::detail {

class TimerEvent : util::Noncopyable {
    friend class Timer;

public:
    TimerEvent(const std::function<void()> &cb, time_t delay, time_t period)
        : cb_(cb)
        , delay_(delay)
        , period_(period) {
        expired_time_ = util::now<util::Time::MilliSecond>() + delay;
    }

    TimerEvent(const std::function<void()> &cb, const std::chrono::milliseconds &delay,
               const std::chrono::milliseconds &period)
        : TimerEvent(cb, delay.count(), period.count()) {}

    void cancel() { cancel_ = true; }

    auto expired_time() const -> time_t { return expired_time_; }

    auto operator<=>(const TimerEvent &other) const {
        return this->expired_time_ <=> other.expired_time_;
    }

    [[nodiscard]]
    friend auto
    operator<=>(const std::shared_ptr<TimerEvent> &lhs, const std::shared_ptr<TimerEvent> &rhs) {
        // never be nullptr
        assert(lhs != nullptr);
        assert(rhs != nullptr);
        return *lhs <=> *rhs;
    }

private:
    void update() { expired_time_ = period_ + util::now<util::Time::MilliSecond>(); }

private:
    time_t                expired_time_{0};
    time_t                delay_{0};
    time_t                period_{0};
    std::function<void()> cb_{};
    std::atomic<bool>     cancel_{false};
};

class Timer : util::Noncopyable {
public:
    Timer()
        : fd_(::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK)) {
        if (fd_ < 0) [[unlikely]] {
            throw std::runtime_error(std::format(
                "call timer_create failed, errorno: {} message: {}", fd_, strerror(errno)));
        }
    }

    ~Timer() { ::close(fd_); }

    void start() {
        task_ = work();
        task_.resume();
    }

    auto add_timer_event(const std::function<void()> &cb, const std::chrono::milliseconds &delay,
                         const std::chrono::milliseconds &period = 0ms)
        -> std::shared_ptr<TimerEvent> {
        auto event = std::make_shared<TimerEvent>(cb, delay, period);
        bool need_update = false;
        {
            std::lock_guard lock(mutex_);
            if (events_.empty() || (*events_.begin())->expired_time_ > event->expired_time_) {
                need_update = true;
            };
            events_.emplace(event);
        }
        if (need_update) {
            update_expired_time();
        }
        return event;
    }

    auto fd() const -> int { return fd_; }

private:
    void update_expired_time() {
        decltype(events_.begin()) it;
        {
            std::lock_guard lock(mutex_);
            it = events_.begin();
            if (it == events_.end()) {
                return;
            }
        }
        auto now = util::now<util::Time::MilliSecond>();
        if ((*it)->expired_time_ < now) {
            return;
        }
        auto       internal = (*it)->expired_time_ - now;
        itimerspec new_value;
        ::memset(&new_value, 0, sizeof(new_value));
        new_value.it_value.tv_sec = internal / 1000;
        new_value.it_value.tv_nsec = internal % 1000 * 1000000;
        if (auto res = ::timerfd_settime(fd_, 0, &new_value, nullptr); res != 0) [[unlikely]] {
            log::zed.error("timerfd_settime failed errorno: {} msg: {}", res, strerror(res));
        }
    }

private:
    auto work() -> Task<void> {
        char buf[8];
        while (true) {
            if (auto n = co_await async::Read<async::Exclusive>(fd_, buf, sizeof(buf), 0);
                n != sizeof(buf)) [[unlikely]] {
                log::zed.error("timer read {}/{} bytes ", n, sizeof(buf));
            }
            std::vector<std::shared_ptr<TimerEvent>> expired_events;
            auto tmp = std::make_shared<TimerEvent>(nullptr, 0, 0);
            {
                std::lock_guard lock(mutex_);
                auto            it = events_.upper_bound(tmp);
                expired_events.insert(expired_events.end(), events_.begin(), it);
                events_.erase(events_.begin(), it);
            }
            std::vector<std::function<void()>> cbs;
            cbs.reserve(expired_events.size());
            for (auto &event : expired_events) {
                if (event->cancel_) {
                    continue;
                } else if (event->period_) {
                    event->update();
                    events_.emplace(event);
                }
                cbs.emplace_back(event->cb_);
            }
            update_expired_time();
            for (auto &cb : cbs) {
                cb();
            }
        }
    }

private:
    Task<void>                                 task_{};
    std::multiset<std::shared_ptr<TimerEvent>> events_{};
    std::mutex                                 mutex_{};
    int                                        fd_{-1};
};

} // namespace zed::async::detail