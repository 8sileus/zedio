#pragma once

#include "zedio/common/error.hpp"
// Linux
#include <sys/socket.h>
#include <sys/un.h>
// C
#include <cstring>
// C++
#include <optional>
#include <string_view>

namespace zedio::net {

class UnixSocketAddr {
    UnixSocketAddr(const std::string_view &path) {
        addr_.sun_family = AF_UNIX;
        std::memcpy(addr_.sun_path, path.data(), path.size());
    }

public:
    UnixSocketAddr(const sockaddr *addr, std::size_t len) {
        std::memcpy(&addr_, addr, len);
    }

    [[nodiscard]]
    auto has_pathname() const noexcept -> bool {
        return addr_.sun_path[0] == '\0';
    }

    [[nodiscard]]
    auto pathname() -> std::optional<std::string_view> {
        if (!has_pathname()) {
            return std::nullopt;
        }
        return addr_.sun_path;
    }

    [[nodiscard]]
    auto sockaddr() const noexcept -> const struct sockaddr * {
        return reinterpret_cast<const struct sockaddr *>(&addr_);
    }

    [[nodiscard]]
    auto sockaddr() noexcept -> struct sockaddr * {
        return reinterpret_cast<struct sockaddr *>(&addr_);
    }

    [[nodiscard]]
    auto length() const noexcept -> socklen_t {
        return sizeof(addr_);
    }

private:
    [[nodiscard]]
    static auto build(std::string_view path) -> std::optional<UnixSocketAddr> {
        if (path.size() >= sizeof(addr_.sun_path)) {
            return std::nullopt;
        }
        return UnixSocketAddr{path};
    }

private:
    sockaddr_un addr_{};
};

} // namespace zedio::net
