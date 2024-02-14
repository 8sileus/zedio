#pragma once

// C
#include <cstring>
// C++
#include <format>
#include <stdexcept>
#include <string>

namespace zed {

class Exception : public std::runtime_error {
public:
    Exception(const std::string &message, int errno)
        : std::runtime_error(
            std::format("{} errno: {}, message {}", message, errno, strerror(errno))) {}

    Exception(const std::string &message)
        : std::runtime_error(message) {}
};

} // namespace zed