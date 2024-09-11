#include "zedio/core.hpp"
#include "zedio/log.hpp"
#include "zedio/sync/condition_variable.hpp"
#include "zedio/sync/latch.hpp"

// C++
#include <queue>

using namespace zedio::async;
using namespace zedio::sync;
using namespace zedio::log;
using namespace zedio;

auto consumer(ConditionVariable &cv,
              Mutex             &mutex,
              bool              &run,
              std::queue<int>   &q,
              Latch             &latch,
              std::size_t       &sum) -> Task<void> {
    while (run) {
        co_await mutex.lock();
        std::unique_lock lock(mutex, std::adopt_lock);
        co_await cv.wait(mutex, [&]() { return !run || !q.empty(); });
        if (!run && q.empty()) {
            break;
        }
        sum += q.front();
        q.pop();
    }
    co_await latch.arrive_and_wait();
    co_return;
}

auto producer(ConditionVariable &cv, Mutex &mutex, bool &run, std::queue<int> &q, std::size_t n)
    -> Task<void> {
    for (auto i = 1; i <= n; i += 1) {
        co_await mutex.lock();
        std::unique_lock lock(mutex, std::adopt_lock);
        q.push(i);
        LOG_INFO("push {}", i);
        cv.notify_one();
    }
    run = false;
    cv.notify_all();
}

auto test(std::size_t n) -> Task<void> {
    ConditionVariable cv;
    Mutex             mutex;
    bool              run = true;
    Latch             latch{11};
    std::queue<int>   q;
    std::size_t       sum = 0;
    for (auto i = 0; i < 10; i++) {
        spawn(consumer(cv, mutex, run, q, latch, sum));
    }
    co_await producer(cv, mutex, run, q, n);
    co_await latch.arrive_and_wait();
    console.info("expected {}, actual {}", n * (n + 1) / 2, sum);
}

auto main() -> int {
    SET_LOG_LEVEL(LogLevel::Trace);
    zedio::runtime::MultiThreadBuilder::options().set_num_workers(4).build().block_on(test(10000));
    return 0;
}