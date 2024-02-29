#include "zedio/core.hpp"

#include <iostream>

using namespace zedio::async;
using namespace zedio::io;
using namespace zedio;

auto interval_print_message(std::string messge, std::chrono::seconds interval, std::size_t cnt)
    -> Task<void> {
    while (cnt--) {
        co_await zedio::io::Sleep(interval);
        std::cout << messge << "\n";
    }
}

auto test() -> Task<void> {
    spawn(interval_print_message("sleep 3s", 3s, 3));
    spawn(interval_print_message("sleep 1s", 1s, 9));
    co_await zedio::io::Sleep(10s);
}

auto main() -> int {
    auto runtime = Runtime::options().set_num_workers(1).build();
    runtime.block_on(test());
    return 0;
}