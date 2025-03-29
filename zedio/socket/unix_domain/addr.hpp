#pragma once

// C
#include <cstring>
// C++
#include <format>
#include <optional>
#include <string_view>
#ifdef __linux__
// Linux
#include <netdb.h>
#include <sys/un.h>
#elif _WIN32
#endif

namespace zedio::socket::unix_domain {

class SocketAddr {
public:
    SocketAddr() = default;

    SocketAddr(std::string_view path) {
        addr_.sun_family = AF_UNIX;
        std::memcpy(addr_.sun_path, path.data(), path.size());
    }

    SocketAddr(const sockaddr *addr, std::size_t len) {
        std::memcpy(&addr_, addr, len);
    }

public:
    [[nodiscard]]
    auto has_pathname() const noexcept -> bool {
        return addr_.sun_path[0] == '\0';
    }

    [[nodiscard]]
    auto family() const noexcept -> int {
        return addr_.sun_family;
    }

    [[nodiscard]]
    auto pathname() const noexcept -> std::string_view {
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
        return std::strlen(addr_.sun_path) + sizeof(addr_.sun_family);
    }

public:
    [[nodiscard]]
    static auto parse(std::string_view path) -> std::optional<SocketAddr> {
        if (path.size() >= sizeof(addr_.sun_path) || path.empty()) {
            return std::nullopt;
        }
        return SocketAddr{path};
    }

private:
    sockaddr_un addr_{};
};

} // namespace zedio::socket::unix_domain

namespace std {

template <>
class formatter<zedio::socket::unix_domain::SocketAddr> {
public:
    constexpr auto parse(format_parse_context &context) {
        auto it{context.begin()};
        auto end{context.end()};
        if (it == end || *it == '}') {
            return it;
        }
        ++it;
        if (it != end && *it != '}') {
            throw format_error("Invalid format specifier for UnixSocketAddr");
        }
        return it;
    }

    auto format(const zedio::socket::unix_domain::SocketAddr &addr, auto &context) const noexcept {
        return format_to(context.out(), "{}", addr.pathname());
    }
};

} // namespace std