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
    co_await time::sleep_until(start);
    LOG_INFO("co_await a passed time");
}

auto main_test() -> Task<void> {
    co_await test();
    co_await time::sleep(5s);
    LOG_INFO("interval {}", 5s);
    co_await test();
}

auto main() -> int {
    SET_LOG_LEVEL(zedio::log::LogLevel::Trace);
    return Runtime::options().scheduler().set_num_workers(1).build().block_on(main_test());
}