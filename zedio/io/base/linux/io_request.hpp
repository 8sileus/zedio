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

template <typename Method, typename... Args>
    requires std::is_invocable_v<Method, HANDLE, Args...>
class IORequest {
public:
    IORequest(Method &&method, Args... args)
        : method{method}
        , args{nullptr, args...} {}

    // Delete copy
    IORequest(const IORequest &other) = delete;
    auto operator=(const IORequest &other) -> IORequest & = delete;

    // IORegistrator(IORegistrator &&other) noexcept
    //     : cb_{std::move(other.cb_)}
    //     , sqe_{other.sqe_} {
    //     io_uring_sqe_set_data(sqe_, &this->cb_);
    //     other.sqe_ = nullptr;
    // }

    // auto operator=(IORegistrator &&other) noexcept -> IORegistrator & {
    //     cb_ = std::move(other.cb_);
    //     sqe_ = other.sqe_;
    //     io_uring_sqe_set_data(sqe_, &this->cb_);
    //     other.sqe_ = nullptr;
    //     return *this;
    // }

public:
    auto await_ready() const noexcept -> bool {
        return false;
    }

    auto await_suspend(std::coroutine_handle<> handle) -> bool {
        auto sqe = runtime::detail::t_ring->get_sqe();
        if (sqe == nullptr) {
            data.result_ = -Error::EmptySqe;
            return false;
        }

        data.handle_ = std::move(handle);
        std::get<0>(args) = runtime::detail::t_ring->get_sqe();
        std::apply(method, args);

        runtime::detail::t_ring->submit();
    }

    template <typename T>
    [[REMEMBER_CO_AWAIT]]
    auto set_timeout_at(this T &self, std::chrono::steady_clock::time_point deadline) noexcept {
        data.deadline_ = deadline;
        return time::detail::Timeout{std::move(*static_cast<T *>(this))};
    }

    template <typename T>
    [[REMEMBER_CO_AWAIT]]
    auto set_timeout(this T &self, std::chrono::milliseconds interval) noexcept {
        return set_timeout_at<T>(std::chrono::steady_clock::now() + interval);
    }

protected:
    IOData                              data{};
    Method                              method;
    std::tuple<io_uring_sqe *, Args...> args;
};

} // namespace zedio::io::detail