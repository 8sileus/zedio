#include "zedio/core.hpp"
#include "zedio/log.hpp"
#include "zedio/sync/latch.hpp"
#include "zedio/time.hpp"

using namespace zedio::async;
using namespace zedio::sync;
using namespace zedio::log;
using namespace zedio;

auto latch_test(Latch &latch_, [[maybe_unused]] std::size_t index, std::atomic<std::size_t> &sum)
    -> Task<void> {
    sum.fetch_add(1, std::memory_order::relaxed);
    // LOG_INFO("latch_test {} suspend", index);
    co_await latch_.arrive_and_wait();
    // LOG_INFO("latch_test {} resume", index);
}

auto test(std::size_t n) -> Task<void> {
    Latch                    latch{static_cast<std::ptrdiff_t>(n)};
    std::atomic<std::size_t> sum = 0;
    assert(n >= 1);
    assert(latch.try_wait() == false);
    for (auto i = 0; i < n - 1; i += 1) {
        spawn(latch_test(latch, i, sum));
    }
    sum.fetch_add(1, std::memory_order::relaxed);
    co_await latch.arrive_and_wait();
    // co_await time::sleep(5s);
    console.info("expected: {}, actual {}", n, sum.load());
    co_return;
}

auto main() -> int {
    SET_LOG_LEVEL(LogLevel::Trace);
    zedio::runtime::MultiThreadBuilder::default_create().block_on(test(100000));
    return 0;
}
