#pragma once

#include <cinttypes>

namespace zedio::io::detail {

struct Config {
public:
    // size of io_uring_queue entries
    std::size_t ring_entries_{1024};
    // io_uring flags
    uint32_t io_uring_flags_{0};
    // how many weak submissions are merged into one strong submission
    // 0: Merge all submission
    uint32_t num_weak_submissions_{4};

    // static
public:
    static constexpr std::size_t LOCAL_QUEUE_CAPACITY{256};

    static constexpr std::size_t DEFAULT_BUF_SIZE{8 * 1024};
};

} // namespace zedio::io::detail