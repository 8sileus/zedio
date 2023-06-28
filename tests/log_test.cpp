#include "log/log.hpp"
#include "singleton.hpp"
#include "time_test.hpp"

#include <sys/resource.h>
#include <iostream>
#include <thread>
#include <vector>

using namespace zed;

auto test_logger = log::Logger("log_test");

void bench(bool longLog) {
    int         cnt = 0;
    const int   kBatch = 1000;
    std::string empty = " ";
    std::string longStr(3000, 'X');
    longStr += " ";

    for (int t = 0; t < 30; ++t) {
        auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < kBatch; ++i) {
            test_logger.info("Hello 0123456789 abcdefghijklmnopqrstuvwxyz" + (longLog ? longStr : empty) + "{}", ++cnt);
        }
        auto end = std::chrono::steady_clock::now();
        std::cout << (end - start).count() << '\n';

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

    util::SpendTime(test, 4, argc > 1);
}