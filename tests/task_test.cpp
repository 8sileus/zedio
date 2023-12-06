#define BOOST_TEST_MODULE task_test

#include "async/task.hpp"

#include <boost/test/included/unit_test.hpp>

int n;

zed::async::Task<int> f2() { co_return 2; }

zed::async::Task<void> f1() { n = co_await f2(); }

zed::async::Task<void> throw_ex() {
    std::cout << "1" << std::endl;
    co_await std::suspend_always{};
    std::cout << "4" << std::endl;
    throw std::runtime_error("test");
}

zed::async::Task<void> throw_ex2() {
    throw std::runtime_error("test");
}

void call_throw_ex(std::coroutine_handle<> handle) {
    std::cout << "3" << std::endl;
    try {
        handle.resume();
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
    auto handle = throw_ex().take();
    handle.resume();
    std::cout << "2" << std::endl;
    try {
        call_throw_ex(handle);
    } catch (std::exception &ex) {
        std::cout << "1: " << ex.what() << std::endl;
        BOOST_CHECK_EQUAL(ex.what(), "test");
    }
}

BOOST_AUTO_TEST_CASE(task_exception2_test) {
    try {
        throw_ex2().take().resume();
    } catch (std::exception &ex) {
        std::cout << "2: " << ex.what() << std::endl;
        BOOST_CHECK_EQUAL(ex.what(), "test");
    }
}

BOOST_AUTO_TEST_SUITE_END()