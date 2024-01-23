#include "zed/common/util/time.hpp"
#include "zed/log.hpp"
// Linux
#include <sys/resource.h>
// C++
#include <iostream>
#include <vector>

using namespace zed;

auto &test_logger = log::make_logger("test_log");

void bench(bool long_flag) {
    int         cnt = 0;
    const int   kBatch = 1000;
    std::string longStr(3000, 'X');
    longStr += " ";

    for (int t = 0; t < 30; ++t) {
        auto start = std::chrono::steady_clock::now();
        if (long_flag) {
            for (int i = 0; i < kBatch; ++i) {
                log::get_logger("test_log")
                    .debug("Hello 0123456789 abcdefghijklmnopqrstuvwxyz {} {} ", longStr, ++cnt);
            }
        } else {
            for (int i = 0; i < kBatch; ++i) {
                log::console.info("Hello 0123456789 abcdefghijklmnopqrstuvwxyz {}", ++cnt);
            }
        }
        auto end = std::chrono::steady_clock::now();
        std::cout << std::format("spend:{}\n", (end - start));
        struct timespec ts = {0, 500 * 1000 * 1000};
        nanosleep(&ts, NULL);
    }
}

void test(int n, bool flag) {
    std::vector<std::thread> vec;
    for (int i = 0; i < n; ++i) {
        vec.emplace_back(bench, flag);
    }
    for (int i = 0; i < n; ++i) {
        vec[i].join();
    }
}

int main(int argc, [[maybe_unused]] char **argv) {
    log::console.setLevel(log::LogLevel::TRACE);
    size_t kOneGB = 1000 * 1024 * 1024;
    rlimit rl = {2 * kOneGB, 2 * kOneGB};
    setrlimit(RLIMIT_AS, &rl);
    std::cout << util::spend_time(test, 3, argc > 1);
    return 0;
}