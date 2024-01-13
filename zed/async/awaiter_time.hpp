#pragma once

#include "zed/async/worker.hpp"
#include "zed/common/error.hpp"
#include "zed/common/macros.hpp"
// C++
#include <chrono>
#include <coroutine>

namespace zed::async::detail {

template <class IOAwaiter>
concept is_awaiter = requires(IOAwaiter awaiter) {
    { awaiter.await_ready() };
    { awaiter.await_resume() };
    { awaiter.await_suspend(std::noop_coroutine()) };
};

class [[REMEMBER_CO_AWAIT]] SleepAwaiter {
public:
    SleepAwaiter(const std::chrono::nanoseconds &timeout)
        : timeout_(timeout) {}

    consteval auto await_ready() const noexcept -> bool {
        return false;
    }

    template <typename T>
    void await_suspend(std::coroutine_handle<T> handle) const noexcept {
        auto cb = [handle]() { handle.resume(); };
        detail::t_worker->timer().add_timer_event(cb, timeout_);
    }

    consteval void await_resume() const noexcept {};

private:
    std::chrono::nanoseconds timeout_{};
};

template <is_awaiter IOAwaiter>
class [[REMEMBER_CO_AWAIT]] TimeoutAwaiter : public IOAwaiter {
public:
    TimeoutAwaiter(IOAwaiter &&op, const std::chrono::nanoseconds &timeout)
        : IOAwaiter{op}
        , event_handle_{
              detail::t_worker->timer().add_timer_event([this]() { cancel_op(); }, timeout)} {}

    auto await_resume() -> decltype(IOAwaiter::await_resume()) {
        event_handle_->cancel();
        if (IOAwaiter::data_.result_ == -ECANCELED || IOAwaiter::data_.result_ == -EINTR) {
            return std::unexpected{make_zed_error(Error::IOtimeout)};
        }
        return IOAwaiter::await_resume();
    }

private:
    void cancel_op() {
        auto sqe = io_uring_get_sqe(t_poller->ring());
        if (sqe != nullptr) [[likely]] {
            io_uring_prep_cancel(sqe, &this->data_, 0);
            io_uring_sqe_set_data(sqe, nullptr);
            io_uring_submit(t_poller->ring());
        }
    }

private:
    std::shared_ptr<TimerEvent> event_handle_;
};

} // namespace zed::async::detail
