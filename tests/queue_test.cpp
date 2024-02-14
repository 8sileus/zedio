#define BOOST_TEST_MODULE ring_buffer_test

#include "zedio/async/queue.hpp"

#include <boost/test/included/unit_test.hpp>

#include <thread>

using namespace zedio::async::detail;

BOOST_AUTO_TEST_SUITE(ring_buffer_test)

BOOST_AUTO_TEST_CASE(local_queue_test) {
    GlobalQueue              gq;
    LocalQueue               lq;
    std::atomic<std::size_t> cnt{0};
    BOOST_CHECK_EQUAL(lq.size(), 0);
    BOOST_CHECK_EQUAL(lq.empty(), true);
    BOOST_CHECK_EQUAL(gq.size(), 0);
    BOOST_CHECK_EQUAL(gq.empty(), true);
    auto ele = std::noop_coroutine();
    constexpr auto len = lq.capacity() + lq.capacity() / 2 + 1;
    for (auto i = 0uz; i < len; ++i) {
        lq.push_back_or_overflow(ele, gq);
    }
    BOOST_CHECK_EQUAL(lq.size(), lq.capacity());
    BOOST_CHECK_EQUAL(lq.empty(), false);
    BOOST_CHECK_EQUAL(gq.size(), lq.capacity() / 2 + 1);
    BOOST_CHECK_EQUAL(gq.empty(), false);
    std::vector<std::thread> threads;
    for (auto i = 0uz; i < 4uz; ++i) {
        threads.emplace_back([&]() {
            std::size_t num{0};
            while (lq.pop().has_value()) {
                ++num;
            }
            cnt += num;
        });
    }
    for (auto &thread : threads) {
        thread.join();
    }
    BOOST_CHECK_EQUAL(lq.capacity(), cnt);
}

BOOST_AUTO_TEST_CASE(global_queue_test) {
    GlobalQueue gq;
    BOOST_CHECK_EQUAL(gq.size(), 0);
    BOOST_CHECK_EQUAL(gq.empty(), true);
    auto ele = std::noop_coroutine();
    std::atomic<std::size_t> cnt{0};
    std::vector<std::thread> threads;
    constexpr std::size_t    num = 512;
    for (auto i = 0uz; i < 4; ++i) {
        threads.emplace_back([&]() {
            for (auto i = 0uz; i < num; ++i) {
                gq.push(ele);
            }
        });
    }
    for(auto&thread:threads){
        thread.join();
    }
    BOOST_CHECK_EQUAL(gq.empty(), false);
    BOOST_REQUIRE_EQUAL(num * threads.size(), gq.size());
    threads.clear();
    for (auto i = 0uz; i < 4; ++i) {
        threads.emplace_back([&]() {
            std::size_t num = 0;
            while (gq.pop().has_value()) {
                ++num;
            }
            cnt += num;
        });
    }
    for(auto&thread:threads){
        thread.join();
    }
    BOOST_REQUIRE_EQUAL(num * threads.size(), cnt);
    BOOST_REQUIRE_EQUAL(0, gq.size());
    BOOST_REQUIRE_EQUAL(true, gq.empty());
}

BOOST_AUTO_TEST_SUITE_END()