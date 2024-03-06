#pragma once

// Linux
#include <sys/time.h>

// C++
#include <format>
#include <functional>

namespace zedio::log::detail {

enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
};

[[nodiscard]]
consteval auto level_to_string(LogLevel level) noexcept -> std::string_view {
    switch (level) {
    case LogLevel::TRACE:
        return "TRACE";
    case LogLevel::DEBUG:
        return "DEBUG";
    case LogLevel::INFO:
        return "INFO ";
    case LogLevel::WARN:
        return "WARN ";
    case LogLevel::ERROR:
        return "ERROR";
    case LogLevel::FATAL:
        return "FATAL";
    default:
        return "Remember implement for LogLevel";
    }
}

[[nodiscard]]
consteval auto level_to_color(LogLevel level) noexcept -> std::string_view {
    switch (level) {
    case LogLevel::TRACE:
        return "\033[1;36m"; // cyan
    case LogLevel::DEBUG:
        return "\033[1;34m"; // blue
    case LogLevel::INFO:
        return "\033[1;32m"; // green
    case LogLevel::WARN:
        return "\033[1;33m"; // yellow
    case LogLevel::ERROR:
        return "\033[1;31m"; // red
    case LogLevel::FATAL:
        return "\033[1;35m"; // purple
    default:
        return "NOT DEFINE COLOR";
    }
}

[[nodiscard]]
consteval auto reset_format() noexcept -> std::string_view {
    return "\033[0m";
}

} // namespace zedio::log::detail
