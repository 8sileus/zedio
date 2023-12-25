#pragma once

#include "common/util/noncopyable.hpp"
// C
#include <cassert>
// C++
#include <coroutine>
#include <stdexcept>
#include <variant>

namespace zed::async {

template <typename T>
class Task;

namespace detail {

    struct TaskPromiseBase {
        struct TaskFinalAwaiter {
            constexpr auto await_ready() const noexcept -> bool { return false; }

            template <typename T>
            auto await_suspend(std::coroutine_handle<T> callee) const noexcept
                -> std::coroutine_handle<> {
                if (callee.promise().m_caller) {
                    return callee.promise().m_caller;
                } else {
                    callee.destroy();
                    return std::noop_coroutine();
                }
            }

            // this function never be called
            constexpr void await_resume() const noexcept {}
        };
        virtual ~TaskPromiseBase() = default;

        constexpr auto initial_suspend() const noexcept -> std::suspend_always { return {}; }

        constexpr auto final_suspend() const noexcept -> TaskFinalAwaiter { return {}; }

        std::coroutine_handle<> m_caller{nullptr};
    };

    template <typename T>
    struct TaskPromise final : public TaskPromiseBase {
        auto get_return_object() noexcept -> Task<T>;

        void unhandled_exception() noexcept {
            m_value.template emplace<std::exception_ptr>(std::current_exception());
        }

        template <typename F>
            requires std::is_convertible_v<F &&, T> && std::is_constructible_v<T, F &&>
        //&& std::is_nothrow_constructible_v<T, F &&>
        void return_value(F &&value) {
            m_value.template emplace<T>(std::forward<F>(value));
        }

        auto result() & -> T & {
            if (std::holds_alternative<std::exception_ptr>(m_value)) [[unlikely]] {
                std::rethrow_exception(std::get<std::exception_ptr>(m_value));
            }
            assert(std::holds_alternative<T>(m_value));
            return std::get<T>(m_value);
        }

        auto result() && -> T && {
            if (std::holds_alternative<std::exception_ptr>(m_value)) [[unlikely]] {
                std::rethrow_exception(std::get<std::exception_ptr>(m_value));
            }
            assert(std::holds_alternative<T>(m_value));
            return std::move(std::get<T>(m_value));
        }

    private:
        std::variant<std::monostate, T, std::exception_ptr> m_value{};
    };

    template <>
    struct TaskPromise<void> final : public TaskPromiseBase {
        auto get_return_object() noexcept -> Task<void>;

        constexpr void return_void() const noexcept {};

        void unhandled_exception() noexcept { m_exception = std::move(std::current_exception()); }

        void result() const {
            if (m_exception != nullptr) [[unlikely]] {
                std::rethrow_exception(m_exception);
            }
        }

    private:
        std::exception_ptr m_exception{nullptr};
    };

} // namespace detail

template <typename T = void>
class [[nodiscard]] Task : util::Noncopyable {
public:
    using promise_type = detail::TaskPromise<T>;

private:
    struct AwaitableBase {
        std::coroutine_handle<promise_type> m_handle;

        AwaitableBase(std::coroutine_handle<promise_type> handle) noexcept
            : m_handle{handle} {}

        auto await_ready() const noexcept -> bool { return !m_handle || m_handle.done(); }

        auto await_suspend(std::coroutine_handle<> caller) -> std::coroutine_handle<> {
            m_handle.promise().m_caller = caller;
            return m_handle;
        }
    };

public:
    Task() noexcept = default;

    explicit Task(std::coroutine_handle<promise_type> handle)
        : m_handle(handle) {}

    ~Task() {
        if (m_handle) {
            m_handle.destroy();
            m_handle = nullptr;
        }
    }

    Task(Task &&other) noexcept
        : m_handle(std::move(other.m_handle)) {
        other.m_handle = nullptr;
    }

    Task &operator=(Task &&other) noexcept {
        if (std::addressof(other) != this) [[likely]] {
            if (m_handle) {
                m_handle.destroy();
            }
            m_handle = std::move(other.m_handle);
            other.m_handle = nullptr;
        }
        return *this;
    }

    auto operator co_await() const & noexcept {
        struct Awaitable final : public AwaitableBase {
            decltype(auto) await_resume() {
                if (!this->m_handle) [[unlikely]] {
                    throw std::logic_error("m_handle is nullptr");
                }
                return this->m_handle.promise().result();
            }
        };
        return Awaitable{m_handle};
    }

    auto operator co_await() && noexcept {
        struct Awaitable final : public AwaitableBase {
            decltype(auto) await_resume() {
                if (!this->m_handle) [[unlikely]] {
                    throw std::logic_error("m_handle is nullptr");
                }
                if constexpr (std::is_same_v<T, void>) {
                    return this->m_handle.promise().result();
                } else {
                    return std::move(this->m_handle.promise().result());
                }
            }
        };
        return Awaitable{m_handle};
    }

    auto take() -> std::coroutine_handle<promise_type> {
        if (m_handle == nullptr) [[unlikely]] {
            throw std::logic_error("m_hanlde is nullptr");
        }
        auto res = std::move(m_handle);
        m_handle = nullptr;
        return res;
    }

    void resume() { m_handle.resume(); }

private:
    std::coroutine_handle<promise_type> m_handle{nullptr};
};

template <typename T>
inline auto detail::TaskPromise<T>::get_return_object() noexcept -> Task<T> {
    return Task<T>{std::coroutine_handle<TaskPromise>::from_promise(*this)};
}

inline auto detail::TaskPromise<void>::get_return_object() noexcept -> Task<void> {
    return Task<void>{std::coroutine_handle<TaskPromise>::from_promise(*this)};
}

} // namespace zed::async
