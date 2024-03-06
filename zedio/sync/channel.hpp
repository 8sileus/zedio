#pragma once

#include "zedio/async/coroutine/task.hpp"
#include "zedio/common/util/ring_buffer.hpp"
#include "zedio/sync/condition_variable.hpp"
#include "zedio/sync/mutex.hpp"

namespace zedio::sync {

template <typename T, std::size_t SIZE>
class Channel {
public:
    auto send(T &&value) -> async::Task<void> {
        co_await mutex_.lock();
        std::lock_guard lock(mutex_, std::adopt_lock);
        senders_.wait(mutex_, [this]() { return !buf_.is_fill(); });

        assert(!buf_.is_fill());
        buf_.safety_push(std::forward<T>(value));
    }

    auto recv() -> async::Task<T> {
        co_await mutex_.lock();
        std::lock_guard lock(mutex_, std::adopt_lock);
        receivers_.wait(mutex_, [this]() { return !buf_.is_empty(); });

        assert(!buf_.is_empty());
        return buf_.safety_pop();
    }

private:
    Mutex                          mutex_{};
    ConditionVariable              senders_{};
    ConditionVariable              receivers_{};
    util::StackRingBuffer<T, SIZE> buf_{};
};

// template <typename T, std::size_t SIZE>
// auto channel() -> std::pair<Channel::Sender, Channel::Receiver> {
//     auto ptr = std::make_shared<Channel<T, SIZE>>();
// }

} // namespace zedio::sync
