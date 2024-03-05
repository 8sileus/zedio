#pragma once

#include "zedio/common/error.hpp"
#include "zedio/common/macros.hpp"
#include "zedio/common/util/noncopyable.hpp"
#include "zedio/io/base/callback.hpp"
#include "zedio/io/base/driver.hpp"

using namespace std::chrono_literals;

namespace zedio::io::detail {

template <class IO>
class IORegistrator {
public:
    template <typename F, typename... Args>
        requires std::is_invocable_v<F, io_uring_sqe *, Args...>
    IORegistrator(F &&f, Args... args)
        : sqe_{t_ring->get_sqe()} {
        if (sqe_ != nullptr) [[likely]] {
            std::invoke(std::forward<F>(f), sqe_, std::forward<Args>(args)...);
            io_uring_sqe_set_data(sqe_, &this->cb_);
        } else {
            cb_.result_ = -Error::EmptySqe;
        }
    }

    // Delete copy
    IORegistrator(const IORegistrator &other) = delete;
    auto operator=(const IORegistrator &other) -> IORegistrator & = delete;
    // Delete move
    IORegistrator(IORegistrator &&other) = delete;
    auto operator=(IORegistrator &&other) -> IORegistrator & = delete;

    auto await_ready() const noexcept -> bool {
        return sqe_ == nullptr;
    }

    void await_suspend(std::coroutine_handle<> handle) {
        cb_.handle_ = std::move(handle);
        assert(sqe_);
        t_ring->submit();
    }

    [[REMEMBER_CO_AWAIT]]
    auto set_timeout_for(std::chrono::nanoseconds timeout) -> IO & {
        if (timeout >= std::chrono::milliseconds{1} && sqe_ != nullptr) [[likely]] {
            auto timeout_sqe = t_ring->get_sqe();
            if (timeout_sqe != nullptr) [[likely]] {
                sqe_->flags |= IOSQE_IO_LINK;

                ts_.tv_sec = timeout.count() / 1000'000'000;
                ts_.tv_nsec = timeout.count() % 1000'000'000;

                io_uring_prep_link_timeout(timeout_sqe, &ts_, 0);
                io_uring_sqe_set_data(timeout_sqe, nullptr);
            }
        } else {
            LOG_ERROR("Set timeout failed");
        }
        return *static_cast<IO *>(this);
    }

    [[REMEMBER_CO_AWAIT]]
    auto set_timeout_at(std::chrono::steady_clock::time_point deadline) -> IO & {
        return set_timeout_for(deadline - std::chrono::steady_clock::now());
    }

protected:
    Callback      cb_{};
    io_uring_sqe *sqe_;

private:
    struct __kernel_timespec ts_;
};

} // namespace zedio::io::detail