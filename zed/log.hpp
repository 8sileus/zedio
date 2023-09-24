#pragma once

#include "log/manager.hpp"
#include "util/singleton.hpp"

namespace zed::log {

auto &console = util::Singleton<detail::ConsoleLogger>::Instance();
auto &manager = util::Singleton<detail::LogManager>::Instance();
auto &zed = console;

static inline auto make_logger(const std::string &name, const std::string &prefixpath = "")
    -> detail::FileLogger & {
    manager.create_logger(name, prefixpath + name);
    auto &log_manager = util::Singleton<detail::LogManager>::Instance();
    log_manager.create_logger(name, prefixpath + name);
    return log_manager.get_logger(name);
}

static inline void delete_logger(const std::string &logger_name) {
    manager.delete_logger(logger_name);
}

static inline auto get_logger(const std::string &logger_name) -> detail::FileLogger & {
    return manager.get_logger(logger_name);
}

} // namespace zed::log