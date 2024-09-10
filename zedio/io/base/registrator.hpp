#pragma once

#include "zedio/common/concepts.hpp"
#include "zedio/common/error.hpp"
#include "zedio/common/macros.hpp"
#include "zedio/common/util/noncopyable.hpp"
#include "zedio/io/base/io_data.hpp"
#include "zedio/runtime/driver.hpp"
#include "zedio/time/timeout.hpp"

using namespace std::chrono_literals;

namespace zedio::io::detail {

template <class IO>
class IORegistrator {
public:
    template <typename F, typename... Args>
        requires std::is_invocable_v<F, io_uring_sqe *, Args...>
    IORegistrator(F &&f, Args... args)
        : sqe_{runtime::detail::t_ring->get_sqe()} {
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

    IORegistrator(IORegistrator &&other) noexcept
        : cb_{std::move(other.cb_)}
        , sqe_{other.sqe_} {
        io_uring_sqe_set_data(sqe_, &this->cb_);
        other.sqe_ = nullptr;
    }

    auto operator=(IORegistrator &&other) noexcept -> IORegistrator & {
        cb_ = std::move(other.cb_);
        sqe_ = other.sqe_;
        io_uring_sqe_set_data(sqe_, &this->cb_);
        other.sqe_ = nullptr;
        return *this;
    }

public:
    auto await_ready() const noexcept -> bool {
        return sqe_ == nullptr;
    }

    void await_suspend(std::coroutine_handle<> handle) {
        assert(sqe_);
        cb_.handle_ = std::move(handle);
        runtime::detail::t_ring->submit();
    }

    [[REMEMBER_CO_AWAIT]]
    auto set_timeout_at(std::chrono::steady_clock::time_point deadline) noexcept {
        cb_.deadline_ = deadline;
        return time::detail::Timeout{std::move(*static_cast<IO *>(this))};
    }

    [[REMEMBER_CO_AWAIT]]
    auto set_timeout(std::chrono::milliseconds interval) noexcept {
        return set_timeout_at(std::chrono::steady_clock::now() + interval);
    }

protected:
    IOData        cb_{};
    io_uring_sqe *sqe_;

    // private:
    //     struct __kernel_timespec ts_;
};

} // namespace zedio::io::detail