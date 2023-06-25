#ifndef ZED_SRC_LOG_LOGGER_HPP_
#define ZED_SRC_LOG_LOGGER_HPP_

#include "log/log_appender.hpp"
#include "log/log_event.hpp"
#include "noncopyable.hpp"

namespace zed::log {

class Logger : util::Noncopyable {
public:
    Logger() : m_appender(new StdoutLogAppender) {}

    Logger(const std::string& basename) : m_appender(new FileLogAppender(basename)) {}

    void log(std::string&& msg) { m_appender->log(std::move(msg)); };

    void setAppender(std::unique_ptr<LogAppender> appender) noexcept { m_appender = std::move(appender); }

    void setLevel(LogLevel level) { m_level = level; }

    LogLevel getLevel() { return m_level; }

    template <typename... Args>
    void debug(const std::string_view& format_str, Args&&... args) {
        format<LogDebug>(format_str, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(const std::string_view& format_str, Args&&... args) {
        format<LogInfo>(format_str, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warn(const std::string_view& format_str, Args&&... args) {
        format<LogWarn>(format_str, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(const std::string_view& format_str, Args&&... args) {
        format<LogError>(format_str, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void fatal(const std::string_view& format_str, Args&&... args) {
        format<LogFatal>(format_str, std::forward<Args>(args)...);
    }

private:
    template <IsLogLevel Level, typename... Args>
    void format(const std::string_view& format_str, Args&&... args) {
        if (m_level <= Level::level) {
            auto cb = [this](std::string&& msg) { this->log(std::move(msg)); };
            detail::LogEvent<Level>(std::move(cb)).format(format_str, std::forward<Args>(args)...);
        }
    }

private:
    LogLevel                     m_level{LogLevel::DEBUG};
    std::unique_ptr<LogAppender> m_appender;
};

}  // namespace zed::log

#endif  // ZED_SRC_LOG_LOGGER_HPP_