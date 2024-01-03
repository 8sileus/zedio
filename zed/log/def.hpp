#pragma once

// Linux
#include <sys/time.h>

// C++
#include <format>
#include <functional>

namespace zed::log {

enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
};

namespace detail {

    static thread_local char   t_time_buffer[64]{};
    static thread_local time_t t_last_second{0};

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
            return "Remenber implement for LogLevel";
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

} // namespace detail

} // namespace zed::log
