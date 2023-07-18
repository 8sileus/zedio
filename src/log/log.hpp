#pragma once

#include "log/manager.hpp"
#include "util/singleton.hpp"

namespace zed::log {

inline auto MakeLogger(const std::string &name, const std::string &prefixpath = "")
    -> detail::FileLogger & {
    auto &log_manager = util::Singleton<detail::LogManager>::Instance();
    log_manager.push(name, prefixpath + name);
    return log_manager.get(name);
}

inline auto GetLogger(const std::string &name) -> detail::FileLogger & {
    return util::Singleton<detail::LogManager>::Instance().get(name);
}

template <typename... Args>
inline void Trace(Args &&...args) {
    util::Singleton<detail::ConsoleLogger>::Instance().trace(std::forward<Args>(args)...);
}

template <typename... Args>
inline void Debug(Args &&...args) {
    util::Singleton<detail::ConsoleLogger>::Instance().debug(std::forward<Args>(args)...);
}

template <typename... Args>
inline void Info(Args &&...args) {
    util::Singleton<detail::ConsoleLogger>::Instance().info(std::forward<Args>(args)...);
}

template <typename... Args>
inline void Warn(Args &&...args) {
    util::Singleton<detail::ConsoleLogger>::Instance().warn(std::forward<Args>(args)...);
}

template <typename... Args>
inline void Error(Args &&...args) {
    util::Singleton<detail::ConsoleLogger>::Instance().error(std::forward<Args>(args)...);
}

template <typename... Args>
inline void Fatal(Args &&...args) {
    util::Singleton<detail::ConsoleLogger>::Instance().fatal(std::forward<Args>(args)...);
}

template <typename... Args>
inline void SetLevel(LogLevel level) {
    util::Singleton<detail::ConsoleLogger>::Instance().setLevel(level);
}

template <typename... Args>
inline auto GetLevel() -> LogLevel {
    return util::Singleton<detail::ConsoleLogger>::Instance().getLevel();
}

} // namespace zed::log
