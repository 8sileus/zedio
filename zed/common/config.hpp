#pragma once

// c
#include <cinttypes>

namespace zed::config {

// async
static constexpr uint64_t    IOURING_POLL_TIMEOUTOUT{5}; // seconds
static constexpr std::size_t IOURING_QUEUE_SIZE{256};

// net
static constexpr std::size_t STREAM_BUFFER_DEFAULT_SIZE{1024};
static constexpr std::size_t EXTRA_BUFFER_SUZE{1024 * 64};

// net


} // namespace zed::config
