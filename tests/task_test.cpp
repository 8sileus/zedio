#define BOOST_TEST_MODULE task_test

#include "async/task.hpp"

#include <boost/test/included/unit_test.hpp>

int n;

zed::async::Task<int> f2() { co_return 2; }

zed::async::Task<void> f1() { n = co_await f2(); }

zed::async::Task<void> throw_ex() { throw std::runtime_error("test"); }

void call_throw_ex(std::coroutine_handle<> handle) {
    auto task = [handle]() { handle.resume(); };
    try {
        task();
    } catch (std ::exception &ex) {
        throw ex;
    }
}

BOOST_AUTO_TEST_SUITE(task_test)

BOOST_AUTO_TEST_CASE(task_normal_test) {
    auto task = f1();
    task.take().resume();
    BOOST_CHECK_EQUAL(n, 2);
}

BOOST_AUTO_TEST_CASE(task_exception_test) {
    try {
        auto task = throw_ex();
        call_throw_ex(task.take());
    } catch (std::exception &ex) {
        BOOST_CHECK_EQUAL(ex.what(), "test");
    }
}

BOOST_AUTO_TEST_SUITE_END()