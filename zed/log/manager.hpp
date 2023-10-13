#pragma once

#include "log/logger.hpp"

// C++
#include <unordered_map>

namespace zed::log::detail {

class LogManager : util::Noncopyable {
public:
    auto make_logger(const std::string &logger_name, const std::string_view &base_name)
        -> FileLogger & {
        loggers_.emplace(logger_name, base_name);
        return loggers_.at(logger_name);
    }

    void delete_logger(const std::string &logger_name) { loggers_.erase(logger_name); }

    auto get_logger(const std::string &logger_name) -> FileLogger & {
        try {
            return loggers_.at(logger_name);
        } catch (...) {
            std::cerr << std::format("[{}] isn't exist in {}\n", logger_name, list_all_logger());
            std::rethrow_exception(std::current_exception());
        }
    }

    auto has_logger(const std::string &logger_name) const -> bool {
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

} // namespace zed::log::detail
