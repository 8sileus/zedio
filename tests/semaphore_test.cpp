#include "zedio/async/sync/semaphore.hpp"
#include "zedio/core.hpp"
#include "zedio/log.hpp"
#include "zedio/time.hpp"

using namespace zedio::async;
using namespace zedio::log;
using namespace zedio;

template <std::ptrdiff_t N>
auto sem_test(CountingSemaphore<N>        &sem,
              [[maybe_unused]] std::size_t index,
              std::atomic<std::size_t>    &sum) -> Task<void> {
    LOG_INFO("sem_test {} suspend", index);
    co_await sem.acquire();
    LOG_INFO("sem_test {} resume", index);
    sum.fetch_add(1, std::memory_order::relaxed);
}

template <std::ptrdiff_t N>
auto test() -> Task<void> {
    CountingSemaphore<N>     sem{0};
    std::atomic<std::size_t> sum = 0;
    for (auto i = 0uz; i < N; i += 1) {
        spawn(sem_test<N>(sem, i, sum));
    }
    co_await time::sleep(1s);
    sem.release(N);
    co_await time::sleep(5s);
    console.info("expected: {}, actual {}", N, sum.load());
    co_return;
}

auto main() -> int {
    SET_LOG_LEVEL(LogLevel::TRACE);
    auto runtime = Runtime::options().set_num_workers(4).build();
    runtime.block_on(test<1000>());
    return 0;
}