#pragma once

#include "zedio/async/coroutine/task.hpp"
#include "zedio/common/debug.hpp"
#include "zedio/common/util/noncopyable.hpp"
#include "zedio/io/base/callback.hpp"
#include "zedio/io/base/poller.hpp"
// Linux
#include <sys/timerfd.h>
// C
#include <cassert>
#include <cstring>
// C++
#include <format>
#include <functional>
#include <memory>
#include <set>
#include <shared_mutex>

using namespace std::literals::chrono_literals;

namespace zedio::io::detail {

class Timer;

thread_local Timer *t_timer;

class TimerEvent : util::Noncopyable {
public:
    TimerEvent(const std::function<void()>    &cb,
               const std::chrono::nanoseconds &delay,
               const std::chrono::nanoseconds &period)
        : cb_(cb)
        , expired_time_{std::chrono::steady_clock::now() + delay}
        , period_{period} {}

    void cancel() {
        cb_ = nullptr;
    }

    [[nodiscard]]
    auto is_canceled() const -> bool {
        return cb_ == nullptr;
    }

    void stop_repeating() {
        period_ = std::chrono::nanoseconds{0};
    }

    [[nodiscard]]
    auto is_repeated() const -> bool {
        return period_.count() != 0;
    }

    [[nodiscard]]
    auto expired_time() const -> const std::chrono::steady_clock::time_point & {
        return expired_time_;
    }

    void execute() {
        assert(cb_ != nullptr);
        cb_();
    }

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

    void update() {
        expired_time_ = std::chrono::steady_clock::now() + period_;
    }

private:
    std::function<void()>                 cb_;
    std::chrono::steady_clock::time_point expired_time_;
    std::chrono::nanoseconds              period_;
};

class Timer : util::Noncopyable {
public:
    Timer()
        : loop_{loop()}
        , fd_{::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK)} {
        if (fd_ < 0) [[unlikely]] {
            throw std::runtime_error(
                std::format("call timer_create failed, error: {}", strerror(errno)));
        }
        assert(t_timer == nullptr);
        t_timer = this;
        loop_.resume();
    }

    ~Timer() {
        // t_poller->unregister_file(idx_);
        t_timer = nullptr;
        ::close(fd_);
    }

    auto add_timer_event(const std::function<void()>    &work,
                         const std::chrono::nanoseconds &delay,
                         const std::chrono::nanoseconds &period = 0us)
        -> std::shared_ptr<TimerEvent> {
        auto event = std::make_shared<TimerEvent>(work, delay, period);
        bool need_update
            = events_.empty() || (*events_.begin())->expired_time() > event->expired_time();
        events_.emplace(event);
        if (need_update) {
            update_expired_time();
        }
        return event;
    }

    [[nodiscard]]
    auto fd() const -> int {
        return fd_;
    }

private:
    void update_expired_time() {
        if (events_.empty()) {
            return;
        }
        auto   first_expired_time = (*events_.begin())->expired_time();
        auto   now = std::chrono::steady_clock::now();
        time_t internal = 1;
        if (first_expired_time < now) {
            // do nothing
        } else {
            internal = static_cast<std::chrono::nanoseconds>(first_expired_time - now).count();
        }
        itimerspec new_value{};
        // ::memset(&new_value, 0, sizeof(new_value));
        new_value.it_value.tv_sec = internal / 1000'000'000;
        new_value.it_value.tv_nsec = internal % 1000'000'000;
        if (::timerfd_settime(fd_, 0, &new_value, nullptr) != 0) [[unlikely]] {
            LOG_ERROR("timerfd_settime failed error: {}", strerror(errno));
        }
        // LOG_TRACE("The next expiration of the timer in {}.{}s later", new_value.it_value.tv_sec,
        //   new_value.it_value.tv_nsec);
    }

private:
    [[nodiscard]]
    auto loop() -> zedio::async::Task<void> {
        char     buf[8]{};
        Callback cb{.handle_ = loop_.handle(), .result_ = 0, .is_exclusive_ = true};
        while (true) {
            auto sqe = t_poller->get_sqe();
            io_uring_prep_read(sqe, fd_, buf, sizeof(buf), 0);
            io_uring_sqe_set_data(sqe, &cb);
            if (auto ret = t_poller->submit(); ret < 0) [[unlikely]] {
                LOG_FATAL("register timer failed: {}", strerror(-ret));
            }
            co_await std::suspend_always{};

            // if (auto result = co_await Read{fd_, buf, sizeof(buf), 0}.set_exclusion();
            //     !result.has_value()) [[unlikely]] {
            //     LOG_ERROR("Timer read failed, error: {}.", result.error().message());
            // }
            auto                                     now = std::chrono::steady_clock::now();
            auto                                     it = events_.begin();
            auto                                     cnt = 0uz;
            std::vector<std::shared_ptr<TimerEvent>> repeated_time_event;
            for (; it != events_.end() && (*it)->expired_time() < now; ++it) {
                auto &event = *it;
                if (event->is_canceled()) {
                    continue;
                }
                event->execute();
                cnt += 1;
                if (event->is_repeated()) {
                    event->update();
                    repeated_time_event.push_back(*it);
                }
            }
            LOG_TRACE("{} timer events expire", cnt);
            events_.erase(events_.begin(), it);
            events_.insert(repeated_time_event.begin(), repeated_time_event.end());
            update_expired_time();
        }
    }

private:
    zedio::async::Task<void>                   loop_;
    std::multiset<std::shared_ptr<TimerEvent>> events_{};
    int                                        fd_;
};

} // namespace zedio::io::detail