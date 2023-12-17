#pragma once

// c
#include <cinttypes>

namespace zed::config {

/// async

// Max capacity of each worker's local queue
static constexpr std::size_t LOCAL_QUEUE_CAPACITY{256};
// How many ticks before polling I/O ?
static constexpr std::size_t EVENT_INTERVAL{61};
// How many ticks before taking a task from global queue?
static constexpr std::size_t CHECK_GLOBAL_QUEUE_INTERVAL{61};
static constexpr std::size_t IOURING_QUEUE_SIZE{256};

/// net
static constexpr std::size_t STREAM_BUFFER_DEFAULT_SIZE{1024};
static constexpr std::size_t EXTRA_BUFFER_SUZE{1024 * 64};

} // namespace zed::config
