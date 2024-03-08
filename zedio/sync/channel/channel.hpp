#pragma once

#include "zedio/async/coroutine/task.hpp"
#include "zedio/common/util/ring_buffer.hpp"
#include "zedio/runtime/worker.hpp"
#include "zedio/sync/mutex.hpp"

namespace zedio::sync::detail {

template <typename T>
class Channel {
    struct SendAwaiter {
        SendAwaiter(Channel &channel, T &value)
            : channel_{channel}
            , value_{value} {}

        auto await_ready() const noexcept -> bool {
            return false;
        }

        auto await_suspend(std::coroutine_handle<> handle) -> bool {
            std::lock_guard lock(channel_.mutex_, std::adopt_lock);
            handle_ = handle;
            if (!channel_.waiting_receivers_.empty()) {
                auto receiver = channel_.waiting_receivers_.front();
                channel_.waiting_receivers_.pop_front();
                receiver->result_ = std::move(value_);
                runtime::detail::t_worker->schedule_task(receiver->handle_);
                result_.emplace();
                return false;
            }
            if (channel_.buf_.is_fill()) {
                if (channel_.is_closed_.load(std::memory_order::acquire)) {
                    return false;
                }
                channel_.waiting_senders_.push_back(this);
                return true;
            } else {
                channel_.buf_.safety_push(std::move(value_));
                result_.emplace();
                return false;
            }
        }

        auto await_resume() const noexcept {
            return result_;
        }

    public:
        Channel                &channel_;
        T                      &value_;
        Result<void>            result_{std::unexpected{make_zedio_error(Error::ClosedChannel)}};
        std::coroutine_handle<> handle_;
    };

    struct RecvAwaiter {
        RecvAwaiter(Channel &channel)
            : channel_{channel} {}

        auto await_ready() const noexcept -> bool {
            return false;
        }

        auto await_suspend(std::coroutine_handle<> handle) -> bool {
            std::lock_guard lock(channel_.mutex_, std::adopt_lock);
            handle_ = handle;
            if (!channel_.waiting_senders_.empty()) {
                auto sender = channel_.waiting_senders_.front();
                channel_.waiting_senders_.pop_front();
                sender->result_.emplace();
                result_.emplace(std::move(sender->value_));
                runtime::detail::t_worker->schedule_task(sender->handle_);
                return false;
            }
            if (channel_.buf_.is_empty()) {
                if (channel_.is_closed_.load(std::memory_order::acquire)) {
                    return false;
                }
                channel_.waiting_receivers_.push_back(this);
                return true;
            } else {
                result_.emplace(channel_.buf_.safety_pop());
                return false;
            }
        }

        auto await_resume() const noexcept -> Result<T> {
            return result_;
        }

    public:
        Channel                &channel_;
        Result<T>               result_{std::unexpected{make_zedio_error(Error::ClosedChannel)}};
        std::coroutine_handle<> handle_;
    };

public:
    using ValueType = T;

    Channel(std::size_t num_senders, std::size_t num_receivers, std::size_t cap)
        : num_senders_{num_senders}
        , num_receivers_{num_receivers}
        , buf_{cap} {}

public:
    auto send(T value) -> async::Task<Result<void>> {
        co_await mutex_.lock();
        co_return co_await SendAwaiter{*this, value};
    }

    auto recv() -> async::Task<Result<T>> {
        co_await mutex_.lock();
        co_return co_await RecvAwaiter{*this};
    }

    void add_sender() {
        num_senders_.fetch_add(1, std::memory_order::relaxed);
    }

    void sub_sender() {
        if (num_senders_.fetch_sub(1, std::memory_order::acq_rel) == 1) {
            destroy();
        }
    }

    void add_receiver() {
        num_receivers_.fetch_add(1, std::memory_order::relaxed);
    }

    void sub_receiver() {
        if (num_senders_.fetch_sub(1, std::memory_order::acq_rel) == 1) {
            destroy();
        }
    }

private:
    void destroy() {
        if (is_closed_.exchange(true, std::memory_order::acq_rel) == false) {
            for (auto sender : waiting_senders_) {
                runtime::detail::t_worker->schedule_task(sender->handle_);
            }
            for (auto receiver : waiting_receivers_) {
                runtime::detail::t_worker->schedule_task(receiver->handle_);
            }
            waiting_senders_.clear();
            waiting_receivers_.clear();
        }
    }

private:
    std::atomic<std::size_t> num_senders_;
    std::atomic<std::size_t> num_receivers_;
    util::HeapRingBuffer<T>  buf_;
    std::list<SendAwaiter *> waiting_senders_{};
    std::list<RecvAwaiter *> waiting_receivers_{};
    std::atomic<bool>        is_closed_{false};
    Mutex                    mutex_{};
};

} // namespace zedio::sync::detail
