#include "zedio/core.hpp"
#include "zedio/time.hpp"

#include <iostream>

using namespace zedio::async;
using namespace zedio::io;
using namespace zedio;

auto interval_print_message([[maybe_unused]] std::string messge,
                            std::chrono::seconds         interval,
                            std::size_t                  cnt) -> Task<void> {
    while (cnt--) {
        co_await time::sleep(interval);
        LOG_INFO("{}", messge);
    }
}

auto test() -> Task<void> {
    spawn(interval_print_message("sleep 3s", 3s, 3));
    spawn(interval_print_message("sleep 1s", 1s, 9));
    auto start = std::chrono::steady_clock::now() + 5s;
    auto interval = time::interval_at(start, 3s);
    co_await interval.tick();
    LOG_INFO("interval {}", 5s);
    co_await interval.tick();
    LOG_INFO("interval {}", 3s);
    co_await interval.tick();
    LOG_INFO("interval {}", 3s);
}

auto main() -> int {
    SET_LOG_LEVEL(zedio::log::LogLevel::TRACE);
    auto runtime = Runtime::options().set_num_workers(1).build();
    runtime.block_on(test());
    return 0;
}