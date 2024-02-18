#include "zedio/async.hpp"
#include "zedio/log.hpp"

using namespace zedio::async;
using namespace zedio::async::sync;
using namespace zedio::log;

auto cal([[maybe_unused]] Mutex &mutex, std::size_t n, std::size_t &sum)
    -> Task<void> {
    while (n--) {
        co_await mutex.lock();
        sum += 1;
        // console.info("sum: {}", sum);
        mutex.unlock();
    }
    co_return;
}

auto test(std::size_t n) -> Task<void> {
    Mutex       mutex_;
    std::size_t sum=0;
    for (auto i = 0uz; i < 10; i += 1) {
        spawn(cal(mutex_, n, sum));
    }
    co_await zedio::async::sleep(10s);
    console.info("expected: {}, actual {}", n * 10, sum);
    co_return;
}

auto main() -> int {
    SET_LOG_LEVEL(LogLevel::TRACE);
    auto runtime = Runtime::options().set_num_worker(4).build();
    runtime.block_on(test(10000));
    return 0;
}