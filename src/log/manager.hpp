#pragma once

#include "log/logger.hpp"

#include <unordered_map>

namespace zed::log::detail {

class LogManager : util::Noncopyable {
public:
    void push(const std::string &name, const std::string_view &base_name) {
        m_loggers.emplace(name, base_name);
    }

    void erase(const std::string &name) { m_loggers.erase(name); }

    auto get(const std::string &name) -> FileLogger & {
        try {
            return m_loggers.at(name);
        } catch (...) {
            std::cerr << std::format("[{}] isn't exist in {}\n", name, listAllName());
            std::rethrow_exception(std::current_exception());
        }
    }

    auto has(const std::string &name) -> bool { return m_loggers.find(name) != m_loggers.end(); }

private:
    // for debug
    auto listAllName() -> std::string {
        std::string str = "[";
        for (const auto &[name, _] : m_loggers) {
            str += name + ",";
        }
        str.back() = ']';
        return str;
    }

private:
    std::unordered_map<std::string, FileLogger> m_loggers;
};

}  // namespace zed::log::detail
