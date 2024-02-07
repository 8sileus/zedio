#pragma once

#include "zed/common/debug.hpp"
#include "zed/common/util/noncopyable.hpp"
// C
#include <cassert>
// C++
#include <coroutine>
#include <optional>
#include <stdexcept>
#include <variant>

namespace zed::async {

template <typename T>
class Task;

namespace detail {

    struct TaskPromiseBase {
        struct TaskFinalAwaiter {
            constexpr auto await_ready() const noexcept -> bool {
                return false;
            }

            template <typename T>
            auto await_suspend(std::coroutine_handle<T> callee) const noexcept
                -> std::coroutine_handle<> {
                if (callee.promise().caller_) {
                    return callee.promise().caller_;
                } else {
#ifdef NEED_ZED_LOG
                    if (callee.promise().ex_ != nullptr) [[unlikely]] {
                        try {
                            std::rethrow_exception(callee.promise().ex_);
                        } catch (const std::exception &ex) {
                            LOG_ERROR("catch a exception {}", ex.what());
                        } catch (...) {
                            LOG_ERROR("catch a unknown exception");
                        }
                    }
#endif
                    callee.destroy();
                    return std::noop_coroutine();
                }
            }

            // this function never be called
            constexpr void await_resume() const noexcept {}
        };

        constexpr auto initial_suspend() const noexcept -> std::suspend_always {
            return {};
        }

        constexpr auto final_suspend() const noexcept -> TaskFinalAwaiter {
            return {};
        }

        void unhandled_exception() noexcept {
            ex_ = std::move(std::current_exception());
            assert(ex_ != nullptr);
        }

        // Record who call me
        std::coroutine_handle<> caller_{nullptr};
        // Record exception
        std::exception_ptr ex_{nullptr};
    };

    template <typename T>
    struct TaskPromise final : public TaskPromiseBase {
        auto get_return_object() noexcept -> Task<T>;

        template <typename F>
            requires std::is_convertible_v<F &&, T> && std::is_constructible_v<T, F &&>
        //&& std::is_nothrow_constructible_v<T, F &&>
        void return_value(F &&value) {
            value_ = std::forward<F>(value);
        }

        auto result() & -> T & {
            if (ex_ != nullptr) [[unlikely]] {
                std::rethrow_exception(ex_);
            }
            assert(value_.has_value());
            return value_.value();
        }

        auto result() && -> T && {
            if (ex_ != nullptr) [[unlikely]] {
                std::rethrow_exception(ex_);
            }
            assert(value_.has_value());
            return std::move(value_.value());
        }

    private:
        std::optional<T> value_{std::nullopt};
    };

    template <>
    struct TaskPromise<void> final : public TaskPromiseBase {
        auto get_return_object() noexcept -> Task<void>;

        constexpr void return_void() const noexcept {};

        void result() const {
            if (ex_ != nullptr) [[unlikely]] {
                std::rethrow_exception(ex_);
            }
        }
    };

} // namespace detail

template <typename T = void>
class [[nodiscard]] Task : util::Noncopyable {
public:
    using promise_type = detail::TaskPromise<T>;

private:
    struct AwaitableBase {
        std::coroutine_handle<promise_type> callee_;

        AwaitableBase(std::coroutine_handle<promise_type> callee) noexcept
            : callee_{callee} {}

        auto await_ready() const noexcept -> bool {
            return !callee_ || callee_.done();
        }

        template <typename PromiseType>
        auto await_suspend(std::coroutine_handle<PromiseType> caller) -> std::coroutine_handle<> {
            callee_.promise().caller_ = caller;
            return callee_;
        }
    };

public:
    Task() noexcept = default;

    explicit Task(std::coroutine_handle<promise_type> handle)
        : handle_(handle) {}

    ~Task() {
        if (handle_) {
            handle_.destroy();
        }
    }

    Task(Task &&other) noexcept
        : handle_(std::move(other.handle_)) {
        other.handle_ = nullptr;
    }

    Task &operator=(Task &&other) noexcept {
        if (std::addressof(other) != this) [[likely]] {
            if (handle_) {
                handle_.destroy();
            }
            handle_ = std::move(other.handle_);
            other.handle_ = nullptr;
        }
        return *this;
    }

    auto operator co_await() const & noexcept {
        struct Awaitable final : public AwaitableBase {
            decltype(auto) await_resume() {
                if (!this->callee_) [[unlikely]] {
                    throw std::logic_error("m_handle is nullptr");
                }
                return this->callee_.promise().result();
            }
        };
        return Awaitable{handle_};
    }

    auto operator co_await() && noexcept {
        struct Awaitable final : public AwaitableBase {
            decltype(auto) await_resume() {
                if (!this->callee_) [[unlikely]] {
                    throw std::logic_error("m_handle is nullptr");
                }
                if constexpr (std::is_same_v<T, void>) {
                    return this->callee_.promise().result();
                } else {
                    return std::move(this->callee_.promise().result());
                }
            }
        };
        return Awaitable{handle_};
    }

    auto take() -> std::coroutine_handle<promise_type> {
        if (handle_ == nullptr) [[unlikely]] {
            throw std::logic_error("m_hanlde is nullptr");
        }
        auto res = std::move(handle_);
        handle_ = nullptr;
        return res;
    }

    void resume() const {
        handle_.resume();
    }

private:
    std::coroutine_handle<promise_type> handle_{nullptr};
};

template <typename T>
inline auto detail::TaskPromise<T>::get_return_object() noexcept -> Task<T> {
    return Task<T>{std::coroutine_handle<TaskPromise>::from_promise(*this)};
}

inline auto detail::TaskPromise<void>::get_return_object() noexcept -> Task<void> {
    return Task<void>{std::coroutine_handle<TaskPromise>::from_promise(*this)};
}

} // namespace zed::async
