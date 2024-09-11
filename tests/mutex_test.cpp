#include "zedio/core.hpp"
#include "zedio/log.hpp"
#include "zedio/sync/mutex.hpp"
#include "zedio/time.hpp"

using namespace zedio::async;
using namespace zedio::sync;
using namespace zedio::log;
using namespace zedio;

auto test_mutex([[maybe_unused]] Mutex &mutex, std::size_t n, std::size_t &sum) -> Task<void> {
    while (n--) {
        co_await mutex.lock();
        sum += 1;
        mutex.unlock();
    }
    co_return;
}

auto test(std::size_t n) -> Task<void> {
    Mutex       mutex_;
    std::size_t sum = 0;
    for (auto i = 0; i < 10; i += 1) {
        spawn(test_mutex(mutex_, n, sum));
    }
    co_await time::sleep(10s);
    console.info("expected: {}, actual {}", n * 10, sum);
    co_return;
}

auto main() -> int {
    SET_LOG_LEVEL(LogLevel::Trace);
    zedio::runtime::MultiThreadBuilder::default_create().block_on(test(100000));
    return 0;
}