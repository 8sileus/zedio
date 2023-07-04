#pragma once

#include "thread.hpp"

#include <format>
#include <functional>
#include <sstream>

namespace zed {

class Logger;

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
};

constexpr auto LevelToString(LogLevel level) -> const char* {
    switch (level) {
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

static thread_local char   t_time_buffer[64];
static thread_local time_t t_last_second{0};

template <LogLevel level>
class LogEvent {
public:
    LogEvent(std::function<void(std::string&&)> cb) : m_cb(cb) { printInfo(); }

    ~LogEvent() {
        m_ss << '\n';
        m_cb(std::move(m_ss.str()));
    }

    [[nodiscard]]
    auto ss() noexcept -> std::stringstream& {
        return m_ss;
    }

    void append(std::string_view str) { m_ss << str; }

    template <typename... Args>
    void format(const std::string_view& fmt, Args&&... args) {
        auto fmt_args{std::make_format_args(args...)};
        m_ss << std::vformat(fmt, fmt_args);
    }

private:
    void printInfo() {
        timeval tv_time;
        ::gettimeofday(&tv_time, nullptr);
        auto cur_second = tv_time.tv_sec;
        auto cur_microsecond = tv_time.tv_usec;
        if (cur_second != t_last_second) {
            t_last_second = cur_second;
            struct tm tm_time;
            ::localtime_r(&cur_second, &tm_time);
            const char* format = "%Y-%m-%d %H:%M:%S";
            ::strftime(t_time_buffer, sizeof(t_time_buffer), format, &tm_time);
        }
        if constexpr (level == LogLevel::DEBUG) {
            m_ss << t_time_buffer << '.' << cur_microsecond << ' ' << LevelToString(level) << ' ' << util::GetTid()
                 << ' ';
        } else {
            m_ss << t_time_buffer << ' ' << LevelToString(level) << ' ' << util::GetTid() << ' ';
        }
    }

private:
    std::function<void(std::string&&)> m_cb;
    std::stringstream                  m_ss;
};

}  // namespace detail

}  // namespace zed
