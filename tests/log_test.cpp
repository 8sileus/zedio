#include "log/log.hpp"
#include "singleton.hpp"
#include "time_test.hpp"

#include <sys/resource.h>
#include <iostream>
#include <thread>
#include <vector>

using namespace zed;
using namespace zed::log;

std::unique_ptr<Logger> test_logger;

void bench(bool longLog) {
    int         cnt = 0;
    const int   kBatch = 1000;
    std::string longStr(3000, 'X');
    longStr += " ";

    for (int t = 0; t < 30; ++t) {
        auto start = std::chrono::steady_clock::now();
        if (longLog) {
            for (int i = 0; i < kBatch; ++i) {
                test_logger->info("Hello 0123456789 abcdefghijklmnopqrstuvwxyz {} {} ", longStr, ++cnt);
            }
        } else {
            for (int i = 0; i < kBatch; ++i) {
                test_logger->info("Hello 0123456789 abcdefghijklmnopqrstuvwxyz {}", ++cnt);
            }
        }
        auto end = std::chrono::steady_clock::now();
        std::cout << std::format("spend:{}\n", std::chrono::duration_cast<std::chrono::milliseconds>(end - start));

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

int main(int argc, char** argv) {
    size_t kOneGB = 1000 * 1024 * 1024;
    rlimit rl = {2 * kOneGB, 2 * kOneGB};
    setrlimit(RLIMIT_AS, &rl);
    if (argc > 1) {
        test_logger.reset(new Logger("log_test"));
    } else {
        test_logger.reset(new Logger);
    }
    util::SpendTime(test, 4, argc > 1);
}