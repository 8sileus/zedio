#pragma once

// C++
#include <thread>

namespace zedio::runtime::detail {

/// Timer
// Max wheel level
static inline constexpr std::size_t MAX_LEVEL = 6;
// size of slot per wheel
static inline constexpr std::size_t SLOT_SIZE{64};

/// Queue
static inline constexpr std::size_t LOCAL_QUEUE_CAPACITY{256};

struct Config {
    // size of io_uring_queue entries
    std::size_t ring_entries_{1024};
    // io_uring flags
    uint32_t io_uring_flags_{0};
    // how many weak submissions are merged into one strong submission
    // 0: Merge all submission
    uint32_t num_weak_submissions_{4};

    // num of worker
    std::size_t num_workers_{std::thread::hardware_concurrency()};
    // How many ticks worker to poll finished I/O operation
    uint32_t check_io_interval_{61};
    // How many ticks worker to pop global queue
    uint32_t check_global_interval_{61};
};

} // namespace zedio::runtime::detail