#include "zedio/core.hpp"
#include "zedio/log.hpp"
#include "zedio/signal.hpp"

#include <thread>

using namespace zedio;

auto signal_test() -> async::Task<void> {
    std::thread t{[]() {
        std::this_thread::sleep_for(5s);
        ::kill(::getpid(), SIGINT);
    }};
    LOG_DEBUG("sigint before");
    co_await signal::ctrl_c();
    LOG_DEBUG("sigint after");
    t.join();
    LOG_DEBUG("join finished");
}

int main() {
    SET_LOG_LEVEL(zedio::log::LogLevel::Trace);
    // auto runtime = zedio::runtime::Builder<>::default_create();
    auto runtime = runtime::CurrentThreadBuilder::default_create();
    runtime.block_on(signal_test());
    return 0;
}