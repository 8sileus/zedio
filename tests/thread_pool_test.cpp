#include "thread.hpp"
#include "time_test.hpp"

#include <format>
#include <iostream>

using namespace zed::util;

unsigned long long  num = 0;
std::mutex mu;

void cal(unsigned long long  l, unsigned long long  r) {
    unsigned long long sum = 0;
    for (unsigned long long i = l; i < r; ++i) {
        sum += i;
    }
    std::lock_guard lock(mu);
    num += sum;
}

void multiple_test(unsigned long long n) {
    ThreadPool pool(2);
    unsigned long long         block = n / 100;
    unsigned long long         remain = n % 100;
    for (int i = 0; i < 100; ++i) {
        auto f = [i, block]() { cal(i * block, (i + 1) * block); };
        pool.push(f);
    }
    pool.push([block, remain]() { cal(100 * block, 100 * block + remain + 1); });
    pool.increase(3);
    pool.wait();
    std::cout << std::format("five thread num = {}\n", num);
}

void single_test(unsigned long long n) {
    unsigned long long sum = 0;
    for (unsigned long long i = 0; i <= n; ++i) {
        sum += i;
    }
    std::cout << std::format("signle thread num = {}\n", sum);
}

int main(int argc, char** argv) {
    if(argc!=2){
        std::cout << std::format("usage ./thread_pool_test num\n");
    }
    long long n = std::stoll(argv[1]);
    SpendTime(multiple_test, n);
    SpendTime(single_test, n);
    return 0;
}