#ifndef ZED_SRC_LOG_LOG_HPP_
#define ZED_SRC_LOG_LOG_HPP_

#include "log/log_appender.hpp"
#include "log/logger.hpp"

namespace zed::log {

Logger default_logger{};
// Logger g_debug_logger{"log_debug"};

}  // namespace zed::log

#endif  // ZED_SRC_LOG_LOG_HPP_