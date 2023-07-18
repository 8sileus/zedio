#define BOOST_TEST_MODULE thread_pool_test

#include "util/thread.hpp"
#include "util/time_test.hpp"

#include <boost/test/included/unit_test.hpp>
#include <random>

using namespace zed::util;

BOOST_AUTO_TEST_SUITE(thread_pool_test)

int MultipleTest(unsigned long long n) {
    ThreadPool         pool(2);
    unsigned long long sum = 0;
    std::mutex         mu;
    auto               cal = [&](unsigned long long l, unsigned long long r) {
        unsigned long long local_sum = 0;
        for (unsigned long long i = l; i < r; ++i) {
            local_sum += i;
        }
        std::lock_guard lock(mu);
        sum += local_sum;
    };
    unsigned long long block = n / 100;
    unsigned long long remain = n % 100;
    for (int i = 0; i < 100; ++i) {
        auto f = [cal, i, block]() { cal(i * block, (i + 1) * block); };
        pool.submit(f);
    }
    pool.submit([cal, block, remain]() { cal(100 * block, 100 * block + remain + 1); });
    pool.increase(3);
    pool.wait();
    return sum;
}

int SingleTest(unsigned long long n) {
    unsigned long long sum = 0;
    for (unsigned long long i = 0; i <= n; ++i) {
        sum += i;
    }
    return sum;
}

BOOST_AUTO_TEST_CASE(cal_test) {
    std::random_device                                rd;
    std::mt19937                                      gen(rd());
    std::uniform_int_distribution<unsigned long long> disturb(1ll, 10000000ll);
    for (int i = 0; i < 100; ++i) {
        unsigned long long num = disturb(gen);
        BOOST_REQUIRE_EQUAL(SingleTest(num), MultipleTest(num));
    }
}

BOOST_AUTO_TEST_SUITE_END()