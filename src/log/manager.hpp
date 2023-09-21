#pragma once

#include "log/logger.hpp"

// C++
#include <unordered_map>

namespace zed::log::detail {

class LogManager : util::Noncopyable {
public:
    void create_logger(const std::string &name, const std::string_view &base_name) {
        loggers_.emplace(name, base_name);
    }

    void delete_logger(const std::string &name) { loggers_.erase(name); }

    auto get_logger(const std::string &name) -> FileLogger & {
        try {
            return loggers_.at(name);
        } catch (...) {
            std::cerr << std::format("[{}] isn't exist in {}\n", name, listAllName());
            std::rethrow_exception(std::current_exception());
        }
    }

    auto has_logger(const std::string &name) -> bool {
        return loggers_.find(name) != loggers_.end();
    }

private:
    // for debug
    auto listAllName() -> std::string {
        std::string str = "[";
        for (const auto &[name, _] : loggers_) {
            str += name + ",";
        }
        str.back() = ']';
        return str;
    }

private:
    std::unordered_map<std::string, FileLogger> loggers_;
};

}  // namespace zed::log::detail
