#pragma once

// Linux
#include <sys/time.h>

// C++
#include <format>
#include <utility>

namespace zedio::log::detail {

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
};

[[nodiscard]]
consteval auto level_to_string(LogLevel level) noexcept -> std::string_view {
    switch (level) {
        using enum LogLevel;
    case Trace:
        return "TRACE";
    case Debug:
        return "DEBUG";
    case Info:
        return "INFO ";
    case Warn:
        return "WARN ";
    case Error:
        return "ERROR";
    case Fatal:
        return "FATAL";
    default:
        std::unreachable();
        return "Remember implement for LogLevel";
    }
}

[[nodiscard]]
consteval auto level_to_color(LogLevel level) noexcept -> std::string_view {
    switch (level) {
        using enum LogLevel;
    case Trace:
        return "\033[97;46m"; // cyan
    case Debug:
        return "\033[97;44m"; // blue
    case Info:
        return "\033[97;42m"; // green
    case Warn:
        return "\033[90;43m"; // yellow
    case Error:
        return "\033[97;41m"; // red
    case Fatal:
        return "\033[97;45m"; // purple
    default:
        std::unreachable();
        return "NOT DEFINE COLOR";
    }
}

[[nodiscard]]
consteval auto reset_format() noexcept -> std::string_view {
    return "\033[0m";
}

} // namespace zedio::log::detail
