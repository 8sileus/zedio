#pragma once

#include "zedio/io/config.hpp"
// C++
#include <thread>

namespace zedio::runtime::detail {

struct Config : public zedio::io::detail::Config {
    /// runtime

    // num of worker
    std::size_t num_workers_{std::thread::hardware_concurrency()};
    // How many ticks worker to poll finished I/O operation
    uint32_t check_io_interval_{61};
    // How many ticks worker to pop global queue
    uint32_t check_global_interval_{61};
};

} // namespace zedio::runtime::detail