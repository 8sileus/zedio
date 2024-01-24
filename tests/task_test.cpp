#include "zed/async/task.hpp"

#include <iostream>

int n;

zed::async::Task<int> f2() { co_return 2; }

zed::async::Task<void> f1() { n = co_await f2(); }

zed::async::Task<void> throw_ex() {
    std::cout << "1\n";
    co_await std::suspend_always{};
    std::cout << "3\n";
    throw std::runtime_error("test");
}

zed::async::Task<void> throw_ex2() {
    throw std::runtime_error("123");
}

void call_throw_ex(std::coroutine_handle<> handle) {
    std::cout << "3" << std::endl;
    try {
        handle.resume();
    } catch (std ::exception &ex) {
        throw ex;
    }
}

int main() {
    auto task = throw_ex();
    auto handle = task.take();
    std::cout << handle.done() << std::endl;
    try {
        handle.resume();
        std::cout << "2\n";
        handle.resume();
        std::cout << "4\n";
    } catch (...) {
        std::cout << "get ex";
        // std::cout << "ex: " << ex.what() << std::endl;
    }
    std::cout << "end" << std::endl;
    return 0;
}