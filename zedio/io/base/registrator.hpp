#pragma once

#include "zedio/common/error.hpp"
#include "zedio/common/macros.hpp"
#include "zedio/common/util/get_func_args.hpp"
#include "zedio/common/util/noncopyable.hpp"
#include "zedio/io/base/callback.hpp"
#include "zedio/io/base/poller.hpp"
#include "zedio/io/base/timeout.hpp"

namespace zedio::io::detail {

template <typename IO, typename IOFunc>
class IORegistrator {
    using Tuple = util::GetFuncArgs<IOFunc>::TupleArgs;

public:
    template <typename... Args>
        requires std::is_invocable_v<IOFunc, io_uring_sqe *, Args...>
    IORegistrator(IOFunc &&f, Args... args)
        : f_{f}
        , args_{t_poller->get_sqe(), std::forward<Args>(args)...} {}

    // Delete copy
    IORegistrator(const IORegistrator &other) = delete;
    auto operator=(const IORegistrator &other) -> IORegistrator & = delete;
    // Delete move
    IORegistrator(IORegistrator &&other) = delete;
    auto operator=(IORegistrator &&other) -> IORegistrator & = delete;

    auto await_ready() const noexcept -> bool {
        return false;
    }

    auto await_suspend(std::coroutine_handle<> handle) -> bool {
        cb_.handle_ = std::move(handle);
        if (std::get<0>(args_) == nullptr) [[unlikely]] {
            t_poller->push_waiting_coro([this](io_uring_sqe *sqe) {
                std::get<0>(this->args_) = sqe;
                std::apply(f_, args_);
                io_uring_sqe_set_data(std::get<0>(args_), &this->cb_);
            });
            return true;
        }
        std::apply(f_, args_);
        io_uring_sqe_set_data(std::get<0>(args_), &this->cb_);
        cb_.result_ = t_poller->submit();
        return cb_.result_ >= 0;
    }

    [[REMEMBER_CO_AWAIT]]
    auto set_exclusion() -> IO & {
        cb_.is_exclusive_ = true;
        return static_cast<IO &>(*this);
    }

    [[REMEMBER_CO_AWAIT]]
    auto set_timeout(std::chrono::nanoseconds timeout) -> TimeoutIO<IO> {
        return TimeoutIO<IO>{std::move(static_cast<IO &>(*this)), timeout};
    }

protected:
    Callback             cb_{};
    std::decay_t<IOFunc> f_;
    Tuple                args_;
};

} // namespace zedio::io::detail