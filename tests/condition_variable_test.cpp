#include "zedio/async.hpp"
#include "zedio/core.hpp"
#include "zedio/log.hpp"

// C++
#include <queue>

using namespace zedio::async;
using namespace zedio::log;
using namespace zedio;

auto consumer(ConditionVariable &cv, [[maybe_unused]] Mutex &mutex, bool &run, std::queue<int> &q)
    -> Task<void> {
    while (run) {
        co_await mutex.lock();
        std::unique_lock lock(mutex, std::adopt_lock);
        co_await cv.wait(mutex, [&]() { return !run || !q.empty(); });
        if (!run) {
            break;
        }
        console.info("pop {}", q.front());
        q.pop();
    }
    co_return;
}

auto producer(ConditionVariable &cv, Mutex &mutex, bool &run, std::queue<int> &q, std::size_t n)
    -> Task<void> {
    for (auto i = 0uz; i < n; i += 1) {
        co_await mutex.lock();
        std::unique_lock lock(mutex, std::adopt_lock);
        q.push(i);
        console.info("push {}", i);
        cv.notify_one();
    }
    run = false;
    cv.notify_all();
}

auto test(std::size_t n) -> Task<void> {
    ConditionVariable cv;
    Mutex             mutex;
    bool              run = true;
    std::queue<int>   q;
    for (auto i = 0; i < 10; i++) {
        spawn(consumer(cv, mutex, run, q));
    }
    co_await producer(cv, mutex, run, q, n);
}

auto main() -> int {
    SET_LOG_LEVEL(LogLevel::TRACE);
    auto runtime = Runtime::options().set_num_worker(4).build();
    runtime.block_on(test(10000));
    return 0;
}