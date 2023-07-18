#pragma once

#include "util/thread.hpp"

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

constexpr auto LevelToString(LogLevel level) noexcept -> std::string_view {
    switch (level) {
    case LogLevel::TRACE:
        return "TRACE";
    case LogLevel::DEBUG:
        return "DEBUG";
    case LogLevel::INFO:
        return "INFO";
    case LogLevel::WARN:
        return "WARN";
    case LogLevel::ERROR:
        return "ERROR";
    case LogLevel::FATAL:
        return "FATAL";
    default:
        return "UNKNOWN";
    }
}

namespace detail {

consteval auto LevelToColor(LogLevel level) noexcept -> std::string_view {
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

static thread_local char   t_time_buffer[64]{};
static thread_local time_t t_last_second{0};

template <LogLevel level>
class LogEvent {
public:
    LogEvent(std::function<void(std::string &&)> cb) : m_cb{cb} {}

    ~LogEvent() { m_cb(std::move(m_msg)); }

    template <typename... Args>
    void format(const std::string_view &fmt, Args &&...args) {
        auto fmt_args{std::make_format_args(args...)};
        m_msg = std::format("{} {}\n", preInfo(), std::vformat(fmt, fmt_args));
    }

private:
    auto preInfo() -> std::string {
        timeval     tv_time;
        std::string str{};

        ::gettimeofday(&tv_time, nullptr);
        auto cur_second = tv_time.tv_sec;
        auto cur_microsecond = tv_time.tv_usec;
        if (cur_second != t_last_second) {
            t_last_second = cur_second;
            struct tm tm_time;
            ::localtime_r(&cur_second, &tm_time);
            const char *format = "%Y-%m-%d %H:%M:%S";
            ::strftime(t_time_buffer, sizeof(t_time_buffer), format, &tm_time);
        }
        if constexpr (level == LogLevel::TRACE) {
            str = std::move(std::format("[{}.{}] [{}] {} ", t_time_buffer, cur_microsecond,
                                        LevelToString(level), util::GetTid()));
        } else {
            str = std::move(
                std::format("[{}] [{}] {} ", t_time_buffer, LevelToString(level), util::GetTid()));
        }
        return str;
    }

private:
    std::function<void(std::string &&)> m_cb{};
    std::string                         m_msg{};
};

} // namespace detail

} // namespace zed::log
