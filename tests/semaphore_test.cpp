#include "zedio/core.hpp"
#include "zedio/log.hpp"
#include "zedio/sync/latch.hpp"
#include "zedio/sync/semaphore.hpp"

using namespace zedio::async;
using namespace zedio::sync;
using namespace zedio::log;
using namespace zedio;

template <std::ptrdiff_t N>
auto count_sem_test(CountingSemaphore<N>        &sem,
                    Latch                       &latch,
                    [[maybe_unused]] std::size_t index,
                    std::atomic<std::size_t>    &sum) -> Task<void> {
    // LOG_INFO("sem_test {} suspend", index);
    co_await sem.acquire();
    // LOG_INFO("sem_test {} resume", index);
    sum.fetch_add(1, std::memory_order::relaxed);
    co_await sem.release();
    latch.count_down();
}

template <std::ptrdiff_t N>
auto test_count_semaphore() -> Task<void> {
    static_assert(N >= 10);
    CountingSemaphore<N>     sem{N / 10};
    Latch                    latch{N + 1};
    std::atomic<std::size_t> sum = 0;
    for (auto i = 0; i < N; i += 1) {
        spawn(count_sem_test<N>(sem, latch, i, sum));
    }

    co_await latch.arrive_and_wait();
    console.info("expected: {}, actual {}", N, sum.load());
    co_return;
}

auto bin_sem_test(BinarySemaphore             &bin_sem,
                  Latch                       &latch,
                  [[maybe_unused]] std::size_t index,
                  std::atomic<std::size_t>    &sum) -> Task<void> {
    // LOG_INFO("sem_test {} suspend", index);
    co_await bin_sem.acquire();
    // LOG_INFO("sem_test {} resume", index);
    sum.fetch_add(1, std::memory_order::relaxed);
    co_await bin_sem.release();
    latch.count_down();
}

template <std::ptrdiff_t N>
auto test_binary_semaphore() -> Task<void> {
    BinarySemaphore          bin_sem;
    Latch                    latch{N + 1};
    std::atomic<std::size_t> sum = 0;
    for (auto i = 0; i < N; i += 1) {
        spawn(bin_sem_test(bin_sem, latch, i, sum));
    }

    co_await latch.arrive_and_wait();
    console.info("expected: {}, actual {}", N, sum.load());
    co_return;
}

auto test() -> Task<void> {
    co_await test_count_semaphore<1000>();
    co_await test_binary_semaphore<1000>();
}

auto main() -> int {
    SET_LOG_LEVEL(LogLevel::Trace);
    runtime::MultiThreadBuilder::default_create().block_on(test());
    return 0;
}
