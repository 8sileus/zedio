#pragma once

#include "zedio/common/util/noncopyable.hpp"
// C++
#include <coroutine>
#include <exception>
#include <iterator>

namespace zedio::async {

template <typename T>
class [[nodiscard]] Generator : util::Noncopyable {

    class Promise {
    public:
        using value_type = std::remove_reference_t<T>;
        using reference_type = std::conditional_t<std::is_reference_v<T>, T, T &>;
        using pointer_type = value_type *;

        auto get_return_object() noexcept -> Generator<T> {
            return Generator<T>{std::coroutine_handle<Promise>::from_promise(*this)};
        }

        constexpr auto initial_suspend() const noexcept -> std::suspend_always {
            return {};
        }

        constexpr auto final_suspend() const noexcept -> std::suspend_always {
            return {};
        }

        auto yield_value(std::remove_reference_t<T> &value) noexcept -> std::suspend_always {
            value_ = std::addressof(value);
            return {};
        }

        auto yield_value(std::remove_reference_t<T> &&value) noexcept -> std::suspend_always {
            value_ = std::addressof(value);
            return {};
        }

        void unhandled_exception() {
            exception_ = std::current_exception();
        }

        void return_void() {}

        auto value() const noexcept -> reference_type {
            return *value_;
        }

        void rethrow_if_exception() {
            if (exception_) {
                std::rethrow_exception(exception_);
            }
        }

    private:
        pointer_type       value_;
        std::exception_ptr exception_;
    };

    struct IteratorSentinel {};

    class Iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Promise::value_type;
        using reference = Promise::reference_type;
        using pointer = Promise::pointer_type;

        Iterator() = default;

        explicit Iterator(std::coroutine_handle<Promise> handle)
            : handle_{handle} {}

        auto operator++() -> Iterator & {
            handle_.resume();
            if (handle_.done()) {
                handle_.promise().rethrow_if_exception();
            }
            return *this;
        }

        void operator++(int) {
            operator++();
        }

        friend auto operator==(const Iterator                          &it,
                               [[maybe_unused]] const IteratorSentinel &sentinel) noexcept -> bool {
            return it.handle_ == nullptr || it.handle_.done();
        }

        friend auto operator!=(const Iterator &it, const IteratorSentinel &sentinel) noexcept
            -> bool {
            return !(it == sentinel);
        }

        friend auto operator==(const IteratorSentinel &sentinel, const Iterator &it) noexcept
            -> bool {
            return (it == sentinel);
        }

        friend auto operator!=(const IteratorSentinel &sentinel, const Iterator &it) noexcept
            -> bool {
            return !(it == sentinel);
        }

        auto operator*() const noexcept -> reference {
            return handle_.promise().value();
        }

        auto operator->() const noexcept -> pointer {
            return std::addressof(operator*());
        }

    private:
        std::coroutine_handle<Promise> handle_{nullptr};
    };

public:
    using promise_type = Promise;
    using iterator = Iterator;

    explicit Generator(std::coroutine_handle<promise_type> handle)
        : handle_{handle} {}

    ~Generator() {
        if (handle_) {
            handle_.destroy();
        }
    }

    Generator(Generator &&other)
        : handle_{other.handle_} {
        other.handle_ = nullptr;
    }

    auto begin() const noexcept -> iterator {
        if (handle_) {
            handle_.resume();
            if (handle_.done()) {
                if (handle_.done()) {
                    handle_.promise().rethrow_if_exception();
                }
            }
        }
        return iterator{handle_};
    }

    auto end() const noexcept -> IteratorSentinel {
        return {};
    }

private:
    std::coroutine_handle<promise_type> handle_;
};

} // namespace zedio::async
