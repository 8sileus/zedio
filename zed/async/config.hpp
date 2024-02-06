#pragma once

#include <thread>

namespace zed::async::detail {

struct Config {
    Config() {}

    Config(const Config &) = default;

    auto operator=(const Config &) -> Config & = default;

    // num of worker
    std::size_t worker_num_{std::thread::hardware_concurrency()};
    // size of io_uring_queue entries
    std::size_t ring_entries_{1024};
    // How many ticks worker to poll finished I/O operation
    uint32_t check_io_interval_{61};
    // How many ticks worker to pop global queue
    uint32_t check_global_interval_{61};
};

} // namespace zed::async::detail