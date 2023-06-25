#ifndef ZED_SRC_LOG_LOGLEVEL_HPP_
#define ZED_SRC_LOG_LOGLEVEL_HPP_

#include <concepts>

namespace zed::log {

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
};

struct LogDebug {
    static constexpr LogLevel    level = LogLevel::DEBUG;
    static constexpr const char* ToString() noexcept { return "DEBUG"; }
};

struct LogInfo {
    static constexpr LogLevel    level = LogLevel::INFO;
    static constexpr const char* ToString() noexcept { return "INFO "; }
};

struct LogWarn {
    static constexpr LogLevel    level = LogLevel::WARN;
    static constexpr const char* ToString() noexcept { return "WARN "; }
};

struct LogError {
    static constexpr LogLevel    level = LogLevel::ERROR;
    static constexpr const char* ToString() noexcept { return "ERROR"; }
};

struct LogFatal {
    static constexpr LogLevel    level = LogLevel::FATAL;
    static constexpr const char* ToString() noexcept { return "FATAL"; }
};

template <class Level>
concept IsLogLevel = requires {
    { Level::ToString() } -> std::same_as<const char*>;
    Level::level;
};

template<IsLogLevel Level>
struct LogLevelTrait{
    LogLevel level = Level::level;
};

}  // namespace zed::log

#endif  // ZED_SRC_LOG_LOGLEVEL_HPP_