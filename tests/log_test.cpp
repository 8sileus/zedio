#include "log/log.hpp"
#include "util/singleton.hpp"
#include "util/time_test.hpp"

#include <sys/resource.h>
#include <iostream>
#include <thread>
#include <vector>

using namespace zed;

auto& test_logger = log::MakeLogger("test_log");

void bench(bool long_flag) {
    int         cnt = 0;
    const int   kBatch = 1000;
    std::string longStr(3000, 'X');
    longStr += " ";

    for (int t = 0; t < 30; ++t) {
        auto start = std::chrono::steady_clock::now();
        if (long_flag) {
            for (int i = 0; i < kBatch; ++i) {
                // test_logger.trace(
                //     "Hello 0123456789 abcdefghijklmnopqrstuvwxyz {} {} ",
                //     longStr, ++cnt);
                log::GetLogger("test_log")
                    .debug("Hello 0123456789 abcdefghijklmnopqrstuvwxyz {} {} ", longStr, ++cnt);
                // test_logger.info(
                //     "Hello 0123456789 abcdefghijklmnopqrstuvwxyz {} {} ",
                //     longStr, ++cnt);
                // test_logger.warn(
                //     "Hello 0123456789 abcdefghijklmnopqrstuvwxyz {} {} ",
                //     longStr, ++cnt);
                // test_logger.error(
                //     "Hello 0123456789 abcdefghijklmnopqrstuvwxyz {} {} ",
                //     longStr, ++cnt);
                // test_logger.fatal(
                //     "Hello 0123456789 abcdefghijklmnopqrstuvwxyz {} {} ",
                //     longStr, ++cnt);
            }
        } else {
            for (int i = 0; i < kBatch; ++i) {
                // log::Trace("Hello 0123456789 abcdefghijklmnopqrstuvwxyz {}",
                //            ++cnt);
                // log::Debug("Hello 0123456789 abcdefghijklmnopqrstuvwxyz {}",
                //            ++cnt);
                log::Info("Hello 0123456789 abcdefghijklmnopqrstuvwxyz {}",
                          ++cnt);
                // log::Warn("Hello 0123456789 abcdefghijklmnopqrstuvwxyz {}",
                //           ++cnt);
                // log::Error("Hello 0123456789 abcdefghijklmnopqrstuvwxyz {}",
                //            ++cnt);
                // log::Fatal("Hello 0123456789 abcdefghijklmnopqrstuvwxyz {}",
                //            ++cnt);
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

int main(int argc, char** argv) {
    log::SetLevel(log::LogLevel::TRACE);
    size_t kOneGB = 1000 * 1024 * 1024;
    rlimit rl = {2 * kOneGB, 2 * kOneGB};
    setrlimit(RLIMIT_AS, &rl);
    util::SpendTime(test, 3, argc > 1);
}