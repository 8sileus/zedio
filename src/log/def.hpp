#pragma once

// Linux
#include <sys/time.h>

// C++
#include <format>
#include <functional>
#include <source_location>

namespace zed::log {

enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
};

constexpr auto level_to_string(LogLevel level) noexcept -> std::string_view {
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
        return "UNKNOWN";
    }
}

namespace detail {

static thread_local char   t_time_buffer[64]{};
static thread_local time_t t_last_second{0};

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

class FmtWithSourceLocation {
public:
    template <typename T>
        requires std::constructible_from<std::string_view, T>
    FmtWithSourceLocation(T &&fmt, std::source_location sl = std::source_location::current())
        : fmt_(std::forward<T>(fmt))
        , sl_(std::move(sl)) {}

    constexpr auto format() const -> const std::string_view & { return fmt_; }

    constexpr auto source_location() const -> const std::source_location & { return sl_; }

private:
    std::string_view     fmt_;
    std::source_location sl_;
};

} // namespace detail

} // namespace zed::log
