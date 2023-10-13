#pragma once

#include "common/util/singleton.hpp"
#include "log/manager.hpp"

namespace zed::log {

auto &console = util::Singleton<detail::ConsoleLogger>::Instance();
auto &zed_logger = console;

static inline auto make_logger(const std::string &name, const std::string &prefixpath = "")
    -> detail::FileLogger & {
    return util::Singleton<detail::LogManager>::Instance().make_logger(name, prefixpath + name);
}

static inline void delete_logger(const std::string &logger_name) {
    util::Singleton<detail::LogManager>::Instance().delete_logger(logger_name);
}

static inline auto get_logger(const std::string &logger_name) -> detail::FileLogger & {
    return util::Singleton<detail::LogManager>::Instance().get_logger(logger_name);
}

static inline auto has_logger(const std::string &logger_name) -> bool {
    return util::Singleton<detail::LogManager>::Instance().has_logger(logger_name);
}

} // namespace zed::log