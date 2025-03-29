#pragma once

#include "zedio/common/concepts.hpp"
#include "zedio/common/error.hpp"
#include "zedio/common/macros.hpp"
#include "zedio/common/util/noncopyable.hpp"
#include "zedio/io/base/io_data.hpp"
#include "zedio/runtime/driver.hpp"
#include "zedio/time/timeout.hpp"

// C++
#include <tuple>

using namespace std::chrono_literals;

namespace zedio::io::detail {

template <typename T>
struct FunctionTraits;

template <typename R, typename... Args>
struct FunctionTraits<R(Args...)> {
    using ReturnType = R;
    using ArgsTuple = std::tuple<Args...>;
};

template <typename R, typename... Args>
struct FunctionTraits<R (*)(Args...)> {
    using ReturnType = R;
    using ArgsTuple = std::tuple<Args...>;
};

template <typename F>
class IORequest {
protected:
    using Function = std::decay_t<F>;
    using ArgsTuple = typename FunctionTraits<Function>::ArgsTuple;

public:
    // template <typenmae... Args>
    // IORequest(Method method, Args... args)
    //     : method{method}
    //     , args{args...} {}

    IORequest(Function function)
        : function{function} {}

    // Delete copy
    IORequest(const IORequest &other) = delete;
    auto operator=(const IORequest &other) -> IORequest & = delete;

    // IORequest(IORequest &&other) noexcept
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

    auto await_suspend(std::coroutine_handle<> coroutine_handle) -> bool {
        data.coroutine_handle = std::move(coroutine_handle);
        data.handle = reinterpret_cast<HANDLE>(std::get<0>(args));
        return call_io_function() == 0;
    }

    auto await_resume() noexcept -> Result<std::size_t> {
        DWORD result = 0;
        ::SetLastError(0);
        if (!::GetOverlappedResult((HANDLE)(std::get<0>(this->args)),
                                   &data.overlapped,
                                   &result,
                                   false)) [[unlikely]] {
            return ::std::unexpected{make_socket_error()};
        }
        return static_cast<std::size_t>(result);
    }

    [[REMEMBER_CO_AWAIT]]
    auto set_timeout_at(this IORequest                       &self,
                        std::chrono::steady_clock::time_point deadline) noexcept {
        data.deadline_ = deadline;
        return time::detail::Timeout{std::move(self)};
    }

    [[REMEMBER_CO_AWAIT]]
    auto set_timeout(this IORequest &self, std::chrono::milliseconds interval) noexcept {
        return self.set_timeout_at(std::chrono::steady_clock::now() + interval);
    }

protected:
    auto call_io_function() -> int {
        if (::CreateIoCompletionPort(reinterpret_cast<HANDLE>(std::get<0>(args)),
                                     runtime::detail::t_ring->get_iocp(),
                                     reinterpret_cast<ULONG_PTR>(&data),
                                     0)
            == nullptr) [[unlikely]] {
            return false;
        }
        std::get<LPOVERLAPPED>(args) = &data.overlapped;
        return std::apply(function, args);
    }

protected:
    IOData    data{};
    Function  function;
    ArgsTuple args;
};

} // namespace zedio::io::detail