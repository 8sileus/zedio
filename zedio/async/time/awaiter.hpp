#pragma once

#include "zedio/async/time/timer.hpp"
#include "zedio/common/concepts.hpp"

namespace zedio::async::detail {

class [[REMEMBER_CO_AWAIT]] SleepAwaiter {
public:
    SleepAwaiter(const std::chrono::nanoseconds &timeout)
        : timeout_(timeout) {}

    auto await_ready() const noexcept -> bool {
        return false;
    }

    void await_suspend(std::coroutine_handle<> handle) const noexcept {
        auto cb = [handle]() { handle.resume(); };
        t_timer->add_timer_event(cb, timeout_);
    }

    void await_resume() const noexcept {};

private:
    std::chrono::nanoseconds timeout_{};
};

template <typename IO>
    requires is_awaiter<IO>
class [[REMEMBER_CO_AWAIT]] TimeoutAwaiter : public IO {
public:
    TimeoutAwaiter(IO &&io, const std::chrono::nanoseconds &timeout)
        : IO{io}
        , event_handle_{t_timer->add_timer_event([this]() { cancel_op(); }, timeout)} {}

    auto await_resume() -> decltype(IO::await_resume()) {
        event_handle_->cancel();
        if (IO::cb_.result_ == -ECANCELED || IO::cb_.result_ == -EINTR) {
            return std::unexpected{make_zedio_error(Error::AsyncTimeout)};
        }
        return IO::await_resume();
    }

private:
    void cancel_op() {
        auto sqe = t_poller->get_sqe();
        if (sqe != nullptr) [[likely]] {
            io_uring_prep_cancel(sqe, &this->data_, 0);
            io_uring_sqe_set_data(sqe, nullptr);
            t_poller->submit();
        }
    }

private:
    std::shared_ptr<TimerEvent> event_handle_;
};

} // namespace zedio::async::detail
