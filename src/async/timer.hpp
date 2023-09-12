#pragma once

#include "async/async_io.hpp"
#include "util/noncopyable.hpp"
#include "util/time.hpp"

// linux
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

namespace zed::async::detail {

class Timer;

class TimerEvent : util::Noncopyable {
    friend class Timer;

public:
    TimerEvent(time_t internal, const std::function<void()> &cb, bool repeat)
        : internal_(internal)
        , cb_(cb)
        , repeat_(repeat) {
        update();
    }

    TimerEvent(const std::chrono::milliseconds &internal, const std::function<void()> &cb,
               bool repeat)
        : TimerEvent(internal.count(), cb, repeat) {}

    /// @brief timer doesn't delete the event immediately,it just assign false to cancel_, when the
    /// event time out tiemr will check the cancel_flag and delete the event if it is true
    void cancel() { cancel_ = true; }

    auto operator<=>(const TimerEvent &other) {
        if (this->expired_time_ == other.expired_time_) {
            return this <=> std::addressof(other);
        }
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
    void update() { expired_time_ = internal_ + util::now<util::Time::MilliSecond>(); }

private:
    time_t                expired_time_{0};
    time_t                internal_{0};
    std::function<void()> cb_{};
    std::atomic<bool>     cancel_{false};
    bool                  repeat_{false};
};

class Timer : util::Noncopyable {
public:
    Timer()
        : fd_(::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK)) {
        if (fd_ < 0) [[unlikely]] {
            // TODO log error
            throw std::runtime_error(std::format(
                "call timer_create failed, errorno: {} message: {}", fd_, strerror(errno)));
        }
    }

    ~Timer() { ::close(fd_); }

    void run() {
        task_ = [this]() -> Task<void> {
            char buf[8];
            while (true) {
                auto res = co_await async::read(fd_, buf, sizeof(buf), 0, false);
                if (res != sizeof(buf)) [[unlikely]] {
                    // TODO log error
                }
                std::vector<std::shared_ptr<TimerEvent>> expired_events;
                {
                    std::lock_guard lock(events_mutex_);
                    auto            tmp = std::make_shared<TimerEvent>(0, nullptr, false);
                    auto            it = events_.upper_bound(tmp);
                    expired_events.insert(expired_events.end(), events_.begin(), it);
                    events_.erase(events_.begin(), it);
                }
                std::vector<std::function<void()>> cbs;
                for (auto &event : expired_events) {
                    if (event->cancel_) {
                        continue;
                    } else if (event->repeat_) {
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
        }();
        task_.resume();
    }

    void add_event(const std::shared_ptr<TimerEvent> &event, bool need_update = true) {
        {
            std::lock_guard lock(events_mutex_);
            if (events_.empty()) {
                need_update = need_update && true;
            } else {
                if ((*events_.begin())->expired_time_ > event->expired_time_) {
                    need_update = need_update && true;
                } else {
                    need_update = false;
                }
            }
            events_.emplace(event);
        }
        if (need_update) {
            // TODO Log error
            update_expired_time();
        }
    }

    auto fd() const -> int { return fd_; }

private:
    void update_expired_time() {
        std::shared_lock lock(events_mutex_);
        auto             it = events_.begin();
        lock.unlock();
        if (it == events_.end()) {
            return;
        }
        auto now = util::now<util::Time::MilliSecond>();
        if ((*it)->expired_time_ < now) {
            return;
        }
        auto     internal = (*it)->expired_time_ - now;
        timespec ts;
        ::memset(&ts, 0, sizeof(ts));
        ts.tv_sec = internal / 1000;
        ts.tv_nsec = (internal % 1000) * 1000000;
        itimerspec new_value;
        ::memset(&new_value, 0, sizeof(new_value));
        new_value.it_value = ts;
        auto res = ::timerfd_settime(fd_, 0, &new_value, nullptr);
        if (res != 0) [[unlikely]] {
            // TODO log error
        }
    }

private:
    Task<void>                            task_;
    std::set<std::shared_ptr<TimerEvent>> events_{};
    std::shared_mutex                     events_mutex_{};
    int                                   fd_{-1};
};

} // namespace zed::async::detail