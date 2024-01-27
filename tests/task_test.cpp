#include "zed/async/task.hpp"
#include "zed/common/debug.hpp"

#include <iostream>

int n;

using namespace zed::async;

zed::async::Task<int> normal_test_2() {
    co_return 2;
}

zed::async::Task<void> normal_test_1() {
    assert(co_await normal_test_2() == 2);
}

zed::async::Task<void> check_exception_2(){
    LOG_DEBUG("2");
    // throw std::runtime_error("test_check_catch_exception");
    throw "test_check_catch_exception";
}

zed::async::Task<void> check_exception_1() {
    LOG_DEBUG("1");
    // try{
    co_await check_exception_2();
    // } catch (const std::exception&ex) {
    // LOG_DEBUG("{}", ex.what());
    // }
    LOG_DEBUG("3");
}

// void test_check_catch_exception() {
//     detail::execute_handle(check_exception_1().take());
// }

int main() {
    {
        auto task = normal_test_1();
        task.resume();
    }
    std::cout << "check exeception\n";
    // test_check_catch_exception();
    return 0;
}