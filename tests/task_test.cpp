#define BOOST_TEST_MODULE task_test

#include "coro/task.hpp"

#include <boost/test/included/unit_test.hpp>

int n;

zed::coro::Task<int> f2() { co_return 2; }

zed::coro::Task<void> f1() { n = co_await f2(); }

BOOST_AUTO_TEST_CASE(task_test) {
    auto task = f1();
    auto t = task.take();
    t.resume();
    BOOST_CHECK_EQUAL(n, 2);
}
