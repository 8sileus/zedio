
#include "zedio/async.hpp"
#include "zedio/log.hpp"

#include <atomic>

using namespace zedio::async;
using namespace zedio::log;

std::atomic<std::size_t> total_sum;

auto send_channel(Channel<std::size_t> &channel, std::size_t start, std::size_t end) -> Task<void> {
    for (auto i = start; i < end; i += 1) {
        console.info("send {}", i);
        co_await channel.send(i);
    }
    console.info("send channel end");
}

auto recv_channel(Channel<std::size_t> &channel) -> Task<void> {
    std::size_t sum = 0;
    while (true) {
        auto ok = co_await channel.recv();
        if (ok) {
            console.info("recv {}", ok.value());
            sum += 1;
        } else {
            break;
        }
    }
    total_sum += sum;
    console.info("recv channel end");
}

auto test_channel() -> Task<void> {
    constexpr std::size_t N = 10000;
    Channel<std::size_t>  channel{};
    for (auto i = 0; i < 10; i += 1) {
        auto n = N / 10;
        spawn(send_channel(channel, n * i, std::min(N + 1, n * (i + 1))));
        spawn(recv_channel(channel));
    }
    co_await zedio::async::sleep(10s);
    channel.close();
    co_await zedio::async::sleep(1s);
    console.info("send: {}, recv: {}", N, total_sum.load());
    co_return;
}

auto consumer(Channel<int> &p1, Channel<int> &p2) -> Task<void> {
    while (true) {
        auto ok = co_await p1.recv();
        co_await p2.send(0);
        if (!ok) {
            break;
        } else {
            console.info("consume {}", ok.value());
        }
    }
}

auto producer() -> Task<void> {
    Channel<int> p1, p2;
    spawn(consumer(p1, p2));
    for (int i = 0; i < 100; i += 1) {
        console.info("produce {}", i);
        co_await p1.send(i);
        co_await p2.recv();
    }
    p1.close();
    p2.close();
}

void p_c_test() {
    auto runtime = Runtime::options().set_num_worker(1).build();
    runtime.block_on(producer());
}

auto main() -> int {
    SET_LOG_LEVEL(zedio::log::LogLevel::TRACE);
    p_c_test();
    console.info("finish p_c_test");
    auto runtime = Runtime::options().set_num_worker(4).build();
    runtime.block_on(test_channel());
    return 0;
}