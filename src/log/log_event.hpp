#ifndef ZED_SRC_LOG_LOGEVENT_HPP_
#define ZED_SRC_LOG_LOGEVENT_HPP_

#include "log/log_level.hpp"
#include "thread.hpp"

#include <cstdio>
#include <functional>
#include <sstream>

namespace zed::log {

class Logger;

namespace detail {

static thread_local char   t_time_buffer[64];
static thread_local time_t t_last_second{0};

template <IsLogLevel Level>
class LogEvent {
public:
    LogEvent(std::function<void(std::string&&)> cb) : m_cb(cb) { printInfo(); }

    ~LogEvent() {
        m_ss << '\n';
        m_cb(std::move(m_ss.str()));
    }

    std::stringstream& ss() { return m_ss; }

    void append(std::string_view str) { m_ss << str; }

    template <typename... Args>
    void format(const std::string_view& format_str, Args&&... args) {
        char buf[1024];
        sprintf(buf, format_str.data(), std::forward<Args>(args)...);
        m_ss << buf;
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
        if constexpr (std::is_same_v<Level, LogDebug>) {
            m_ss << t_time_buffer << '.' << cur_microsecond << ' ' << Level::Tostring() << ' ' << util::getTid << ' ';
        } else {
            m_ss << t_time_buffer << ' ' << Level::ToString() << ' ' << util::getTid << ' ';
        }
    }

private:
    std::function<void(std::string&&)> m_cb;
    std::stringstream m_ss;
};

}  // namespace detail

}  // namespace zed::log

#endif  // ZED_SRC_LOG_LOGEVENT_HPP_