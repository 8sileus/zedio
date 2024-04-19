#pragma once

// C++
#include <coroutine>
#include <list>
#include <mutex>
#include <optional>
#include <queue>

namespace zedio::runtime::current_thread {

class GlobalQueue {
public:
    void push(std::coroutine_handle<> handle) {
        if (is_closed_.load(std::memory_order::acquire)) [[unlikely]] {
            return;
        }
        std::lock_guard lock{mutex_};
        tasks_.push_back(handle);
    }

    void push_batch(std::list<std::coroutine_handle<>> &&handles) {
        if (is_closed_.load(std::memory_order::acquire)) [[unlikely]] {
            return;
        }
        std::lock_guard lock{mutex_};
        tasks_.splice(tasks_.end(), handles);
    }

    [[nodiscard]]
    auto pop() -> std::optional<std::coroutine_handle<>> {
        std::optional<std::coroutine_handle<>> result{std::nullopt};

        if (is_closed_.load(std::memory_order::acquire)) [[unlikely]] {
            return result;
        }

        {
            std::lock_guard lock{mutex_};
            if (!tasks_.empty()) {
                result = tasks_.front();
                tasks_.pop_front();
            }
        }
        return result;
    }

    [[nodiscard]]
    auto pop_all() -> std::list<std::coroutine_handle<>> {
        std::list<std::coroutine_handle<>> result{};
        if (is_closed_.load(std::memory_order::acquire)) [[likely]] {
            return result;
        }
        std::lock_guard lock{mutex_};
        tasks_.swap(result);
        return result;
    }

    [[nodiscard]]
    auto close() -> bool {
        if (is_closed_.load(std::memory_order::acquire)) {
            return false;
        }
        is_closed_.store(true, std::memory_order::release);
        return true;
    }

    [[nodiscard]]
    auto is_closed() -> bool {
        return is_closed_.load(std::memory_order::acquire);
    }

    [[nodiscard]]
    auto empty() -> bool {
        std::lock_guard lock{mutex_};
        return tasks_.empty();
    }

    [[nodiscard]]
    auto size() -> std::size_t {
        std::lock_guard lock{mutex_};
        return tasks_.size();
    }

private:
    std::list<std::coroutine_handle<>> tasks_{};
    std::mutex                         mutex_{};
    std::atomic<bool>                  is_closed_{false};
};

class LocalQueue {
public:
    void push_back_or_overflow(std::coroutine_handle<> task, GlobalQueue &) {
        tasks_.push(task);
    }

    void push(std::coroutine_handle<> task) {
        tasks_.push(task);
    };

    void push_batch(const std::list<std::coroutine_handle<>> &handles) {
        for (const auto &handle : handles) {
            tasks_.push(handle);
        }
    }

    [[nodiscard]]
    auto pop() -> std::optional<std::coroutine_handle<>> {
        std::optional<std::coroutine_handle<>> result{std::nullopt};
        if (!tasks_.empty()) {
            result = tasks_.front();
            tasks_.pop();
        }
        return result;
    }

    [[nodiscard]]
    auto size() const noexcept -> std::size_t {
        return tasks_.size();
    }

    [[nodiscard]]
    auto empty() const noexcept -> bool {
        return tasks_.empty();
    }

private:
    std::queue<std::coroutine_handle<>> tasks_{};
};

} // namespace zedio::runtime::current_thread
