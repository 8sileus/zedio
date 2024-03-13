#include "zedio/core.hpp"
#include "zedio/log.hpp"
#include "zedio/sync/channel.hpp"
#include "zedio/sync/latch.hpp"
#include "zedio/time.hpp"

using namespace zedio::async;
using namespace zedio::sync;
using namespace zedio::log;
using namespace zedio;

auto test_channel(std::size_t n, std::ptrdiff_t num_p, std::ptrdiff_t num_c) -> Task<void> {
    auto [sender, receiver] = Channel<std::size_t>::make(100);
    auto                     latch = Latch{num_c + num_p + 1};
    std::atomic<std::size_t> sum;

    auto p = [&](auto sender, auto &latch, std::size_t n) -> Task<void> {
        for (auto i = 1uz; i <= n; i += 1) {
            if (auto ret = co_await sender.send(i); !ret) {
                LOG_ERROR("{}", ret.error());
                break;
            }
        }
        sender.close();
        co_await latch.arrive_and_wait();
    };
    auto c = [&](auto receiver, auto &latch) -> Task<void> {
        while (true) {
            if (auto ret = co_await receiver.recv(); !ret) {
                LOG_ERROR("{}", ret.error());
                break;
            } else {
                sum.fetch_add(ret.value(), std::memory_order::relaxed);
                // LOG_DEBUG("recv {}", ret.value());
            }
        }
        co_await latch.arrive_and_wait();
    };
    for (auto i = 1; i < num_p; i++) {
        spawn(p(sender, latch, n));
    }
    for (auto i = 1; i < num_c; i++) {
        spawn(c(receiver, latch));
    }
    spawn(p(std::move(sender), latch, n));
    spawn(c(std::move(receiver), latch));
    co_await latch.arrive_and_wait();
    LOG_INFO("expected {}, actual {}", n * (n + 1) / 2 * num_p, sum.load());
}
auto test(std::size_t n) -> Task<void> {
    // test mpmc
    co_await test_channel(n, 4, 4);
    // test mpsc
    co_await test_channel(n, 4, 1);
    // test spmc
    co_await test_channel(n, 1, 4);
    // test mpsc
    co_await test_channel(n, 1, 1);

    auto [sender, receiver] = Channel<std::string>::make(0);
    auto latch = std::make_shared<Latch>(3);
    spawn([](auto sender, auto latch) -> Task<void> {
        for (auto i = 0; i < 10; i += 1) {
            if (auto ret = co_await sender.send(std::string("ping ") + std::to_string(i)); !ret) {
                LOG_ERROR("{}", ret.error());
                break;
            } else {
                LOG_INFO("send ping {}", i);
            }
        }
        sender.close();
        co_await latch->arrive_and_wait();
    }(std::move(sender), latch));
    spawn([](auto receiver, auto latch) -> Task<void> {
        while (true) {
            if (auto ret = co_await receiver.recv(); !ret) {
                LOG_ERROR("{}", ret.error());
                break;
            } else {
                LOG_INFO("recv {}", ret.value());
            }
        }
        receiver.close();
        co_await latch->arrive_and_wait();
    }(std::move(receiver), latch));
    co_await latch->arrive_and_wait();
}

auto main() -> int {
    SET_LOG_LEVEL(LogLevel::TRACE);
    auto runtime = Runtime::options().scheduler().set_num_workers(4).build();
    runtime.block_on(test(1000));
    return 0;
}