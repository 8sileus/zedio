#pragma once

#include "zedio/common/error.hpp"
#include "zedio/common/util/get_func_args.hpp"
#include "zedio/io/base/callback.hpp"
#include "zedio/io/base/poller.hpp"

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
                cb_.result_ = t_poller->submit();
            });
            return true;
        }
        std::apply(f_, args_);
        io_uring_sqe_set_data(std::get<0>(args_), &this->cb_);
        cb_.result_ = t_poller->submit();
        return cb_.result_ >= 0;
    }

    auto set_exclusion() -> IO & {
        cb_.is_exclusive_ = true;
        return static_cast<IO &>(*this);
    }

protected:
    Callback cb_{};

private:
    std::decay_t<IOFunc> f_;
    Tuple                args_;
};

} // namespace zedio::io::detail