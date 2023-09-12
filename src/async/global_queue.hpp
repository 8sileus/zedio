#pragma once

#include <singleton.hpp>
#include <util/thread.hpp>
// C++
#include <coroutine>
#include <mutex>
#include <optional>
#include <queue>

namespace zed::async::detail {

class CoroQueue {
public:
    void push(const std::coroutine_handle<> &coro) {
        std::lock_guard lock(mutex_);
        coros_.push(coro);
    }

    auto take() -> std::optional<std::coroutine_handle<>> {
        std::lock_guard                        lock(mutex_);
        std::optional<std::coroutine_handle<>> res;
        if (!coros_.empty()) {
            res.emplace(std::move(coros_.front()));
            coros_.pop();
        }
        return res;
    }

private:
    util::SpinMutex                     mutex_;
    std::queue<std::coroutine_handle<>> coros_;
};

auto &g_coros = zed::util::Singleton<CoroQueue>::Instance();

} // namespace zed::async::detail