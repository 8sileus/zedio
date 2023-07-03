#pragma once

#include "log/log_appender.hpp"
#include "log/log_event.hpp"
#include "noncopyable.hpp"

namespace zed::log {

class Logger : util::Noncopyable {
public:
    Logger() {}

    Logger(std::unique_ptr<LogAppender> appender, LogLevel level = LogLevel::DEBUG)
        : m_appender(std::move(appender)), m_level(level) {}

    Logger(const std::string_view& name, LogLevel level = LogLevel::DEBUG)
        : m_appender(std::make_unique<FileLogAppender>(name)), m_level(level) {}

    Logger(Logger&& other) noexcept : m_appender(std::move(other.m_appender)), m_level(other.m_level) {}

    auto operator=(Logger&& other) noexcept -> Logger& {
        m_appender = std::move(other.m_appender);
        m_level = other.m_level;
        return *this;
    }

    void setAppender(std::unique_ptr<LogAppender> appender) noexcept { m_appender = std::move(appender); }

    void setLevel(LogLevel level) noexcept { m_level = level; }

    [[nodiscard]]
    auto getLevel() const noexcept -> LogLevel {
        return m_level;
    }

    template <typename... Args>
    void debug(const std::string_view& fmt, Args&&... args) {
        format<LogLevel::DEBUG>(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(const std::string_view& fmt, Args&&... args) {
        format<LogLevel::INFO>(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warn(const std::string_view& fmt, Args&&... args) {
        format<LogLevel::WARN>(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(const std::string_view& fmt, Args&&... args) {
        format<LogLevel::ERROR>(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void fatal(const std::string_view& fmt, Args&&... args) {
        format<LogLevel::FATAL>(fmt, std::forward<Args>(args)...);
    }

private:
    template <LogLevel level, typename... Args>
    void format(const std::string_view& fmt, Args&&... args) {
        if (level >= m_level) {
            auto cb = [this](std::string&& msg) { this->m_appender->log(std::move(msg)); };
            detail::LogEvent<level>(std::move(cb)).format(fmt, std::forward<Args>(args)...);
        }
    }

private:
    std::unique_ptr<LogAppender> m_appender{std::make_unique<StdoutLogAppender>()};
    LogLevel                     m_level{LogLevel::DEBUG};
};

}  // namespace zed::log
