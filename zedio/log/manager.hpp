#pragma once

#include "zedio/log/logger.hpp"

// C++
#include <unordered_map>

namespace zedio::log {

namespace detail {
    class LogManager : util::Noncopyable {
    public:
        auto make_logger(const std::string &logger_name, std::string_view file_base_name)
            -> FileLogger & {
            loggers_.emplace(logger_name, file_base_name);
            return loggers_.at(logger_name);
        }

        void delete_logger(const std::string &logger_name) {
            loggers_.erase(logger_name);
        }

        /// Remember to check whether the return value is nullptr
        [[nodiscard]]
        auto get_logger(const std::string &logger_name) -> FileLogger * {
            if (this->contains_logger(logger_name)) {
                return std::addressof(loggers_.at(logger_name));
            } else {
                return nullptr;
            }
        }

        [[nodiscard]]
        auto contains_logger(const std::string &logger_name) const -> bool {
            return loggers_.find(logger_name) != loggers_.end();
        }

    private:
        // for debug
        auto list_all_logger() -> std::string {
            std::string str = "[";
            for (const auto &[logger_name, _] : loggers_) {
                str += logger_name + ",";
            }
            str.back() = ']';
            return str;
        }

    private:
        std::unordered_map<std::string, FileLogger> loggers_;
    };

} // namespace detail

auto &g_manager = util::Singleton<detail::LogManager>::instance();

static inline auto make_logger(const std::string &logger_name,
                               const std::string &file_base_name = "") -> detail::FileLogger & {
    return g_manager.make_logger(logger_name,
                                 file_base_name.empty() ? logger_name : file_base_name);
}

static inline void delete_logger(const std::string &logger_name) {
    g_manager.delete_logger(logger_name);
}

[[nodiscard]]
static inline auto get_logger(const std::string &logger_name) {
    return g_manager.get_logger(logger_name);
}

[[nodiscard]]
static inline auto contains_logger(const std::string &logger_name) -> bool {
    return g_manager.contains_logger(logger_name);
}

} // namespace zedio::log
