#pragma once

// C++
#include <atomic>
#include <coroutine>
#include <list>
#include <mutex>
#include <optional>

// TODO future: optimize the global queue
// consider unlock structure

namespace zed::async::detail {

class GlobalQueue {
public:
    [[nodiscard]]
    auto size() -> std::size_t {
        return num_.load(std::memory_order::acquire);
    }

    [[nodiscard]]
    auto empty() -> bool {
        return num_ == 0;
    }

    void push(std::coroutine_handle<> &&task) {
        std::lock_guard lock(mutex_);
        if (is_closed_) {
            return;
        }
        tasks_.push_back(task);
        num_.fetch_add(1, std::memory_order::release);
    }

    void push(std::list<std::coroutine_handle<>> &&tasks, std::size_t n) {
        std::lock_guard lock(mutex_);
        tasks_.splice(tasks_.end(), tasks);
        num_.fetch_add(n, std::memory_order::seq_cst);
    }

    [[nodiscard]]
    auto pop() -> std::optional<std::coroutine_handle<>> {
        std::lock_guard lock(mutex_);
        if (tasks_.empty()) {
            return std::nullopt;
        }
        auto result = std::move(tasks_.front());
        tasks_.pop_front();
        num_.fetch_sub(1, std::memory_order::release);
        return result;
    }

    [[nodiscard]]
    auto pop_n(std::size_t &n) -> std::list<std::coroutine_handle<>> {
        std::lock_guard lock(mutex_);

        n = std::min(num_.load(std::memory_order::relaxed), n);
        num_.fetch_sub(n, std::memory_order::release);

        std::list<std::coroutine_handle<>> result;

        auto end = tasks_.begin();
        std::advance(end, n);
        result.splice(result.end(), tasks_, tasks_.begin(), end);
        return result;
    }

    [[nodiscard]]
    auto close() -> bool {
        std::lock_guard lock{mutex_};
        if(is_closed_){
            return false;
        }
        is_closed_ = true;
        return true;
    }

    [[nodiscard]]
    auto is_closed() -> bool {
        std::lock_guard lock{mutex_};
        return is_closed_;
    }

private:
    std::list<std::coroutine_handle<>> tasks_{};
    std::atomic<std::size_t>           num_{0};
    std::mutex                         mutex_{};
    bool                               is_closed_{false};
};

} // namespace zed::async::detail
