#pragma once

#include "zedio/async/io/callback.hpp"
#include "zedio/async/poller.hpp"
// C++
#include <tuple>

namespace zedio::async::detail {

template <Mode mode, typename R>
class IORegistrator {
public:
    template <typename F, typename... Args>
    IORegistrator(F &&f, Args &&...args)
        : cb_{mode}
        , sqe_{t_poller->get_sqe()} {
        if (sqe_ == nullptr) [[unlikely]] {
            return;
        }
        f(sqe_, std::forward<Args>(args)...);
        if constexpr (mode == Mode::F) {
            sqe_->flags |= IOSQE_FIXED_FILE;
        }
        io_uring_sqe_set_data(sqe_, &this->cb_);
    }

    // Delete copy
    IORegistrator(const IORegistrator &) = delete;
    auto operator=(const IORegistrator &) -> IORegistrator & = delete;

    // Delete move
    IORegistrator(IORegistrator &&) = delete;
    auto operator=(IORegistrator &&) -> IORegistrator & = delete;

    auto await_ready() const noexcept -> bool {
        return false;
    }

    auto await_suspend(std::coroutine_handle<> handle) -> bool {
        cb_.handle_ = std::move(handle);
        cb_.result_ = t_poller->submit();
        if (cb_.result_ < 0) {
            return false;
        } else {
            return true;
        }
    }

    auto await_resume() const noexcept -> Result<R> {
        if (cb_.result_ >= 0) [[likely]] {
            if constexpr (std::is_same_v<void, R>) {
                return {};
            } else {
                return static_cast<R>(cb_.result_);
            }
        }
        if (sqe_ == nullptr) [[unlikely]] {
            return std::unexpected{make_zedio_error(Error::NullSeq)};
        }
        return std::unexpected{make_sys_error(-cb_.result_)};
    }

private:
    Callback      cb_;
    io_uring_sqe *sqe_;
};

} // namespace zedio::async::detail