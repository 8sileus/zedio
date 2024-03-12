#pragma once

#include "zedio/runtime/queue.hpp"
// C
#include <cassert>
// C++
#include <chrono>
#include <coroutine>
#include <memory>

namespace zedio::time::detail {

class Entry {
public:
    Entry(std::chrono::steady_clock::time_point expiration_time, std::coroutine_handle<> handle)
        : expiration_time_{expiration_time}
        , handle_{handle} {}

public:
    void execute(runtime::detail::LocalQueue  &local_queue,
                 runtime::detail::GlobalQueue &global_queue) {
        assert(handle_ != nullptr);
        local_queue.push_back_or_overflow(handle_, global_queue);
    }

public:
    [[nodiscard]]
    static auto make(std::chrono::steady_clock::time_point expiration_time,
                     std::coroutine_handle<>               handle)
        -> std::pair<std::shared_ptr<Entry>, std::weak_ptr<Entry>> {
        auto entry = std::make_shared<Entry>(expiration_time, handle);
        return std::make_pair(entry, std::weak_ptr<Entry>{entry});
    }

public:
    std::chrono::steady_clock::time_point expiration_time_;
    std::coroutine_handle<>               handle_{nullptr};
    std::shared_ptr<Entry>                next_{nullptr};
};

} // namespace zedio::time::detail
