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
    // LOG_INFO("latch_test {} suspend", index);
    co_await latch_.arrive_and_wait();
    // LOG_INFO("latch_test {} resume", index);
    sum.fetch_add(1, std::memory_order::relaxed);
}

auto test(std::size_t n) -> Task<void> {
    Latch                    latch{static_cast<std::ptrdiff_t>(n)};
    std::atomic<std::size_t> sum = 0;
    for (auto i = 0uz; i < n - 1; i += 1) {
        spawn(latch_test(latch, i, sum));
    }
    co_await latch.arrive_and_wait();
    sum.fetch_add(1, std::memory_order::relaxed);
    co_await time::sleep(5s);
    console.info("expected: {}, actual {}", n, sum.load());
    co_return;
}

auto main() -> int {
    SET_LOG_LEVEL(LogLevel::TRACE);
    auto runtime = Runtime::options().set_num_workers(4).build();
    runtime.block_on(test(100000));
    return 0;
}