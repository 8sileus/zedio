#pragma once

#include "zedio/runtime/timer/timer.hpp"

namespace zedio::time {

namespace detail {

    template <class T>
    class Timeout : public T {
    public:
        Timeout(T &&io)
            : T{std::move(io)} {}

    public:
        auto await_suspend(std::coroutine_handle<> handle) -> bool {
            assert(this->cb_.entry_ != nullptr);

            auto ret = runtime::detail::t_timer->add_entry(this->cb_.deadline_, &this->cb_);
            if (ret) [[likely]] {
                this->cb_.entry_ = ret.value();
                T::await_suspend(handle);
                return true;
            } else {
                this->cb_.result_ = -ret.error().value();
                io_uring_prep_nop(this->sqe_);
                io_uring_sqe_set_data(this->sqe_, nullptr);
                return false;
            }
        }
    };

} // namespace detail

template <class T>
auto timeout(T &&io, std::chrono::steady_clock::time_point deadline) {
    return io.set_timeout(deadline);
}

template <class T>
auto timeout(T &&io, std::chrono::milliseconds interval) {
    return timeout(std::move(io), std::chrono::steady_clock::now() + interval);
}

} // namespace zedio::time
