#pragma once

// Linux
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
// C
#include <cassert>
#include <cstdint>
#include <cstring>
// C++
#include <format>
#include <memory>
#include <string_view>

namespace zed::net {

class Address {
private:
    Address(const std::string_view &hostname, const std::string_view &service, int flags) {
        addrinfo hints{.ai_flags{flags}};

        addrinfo *result{nullptr};
        if (::getaddrinfo(hostname.data(), service.data(), &hints, &result)) [[unlikely]] {
            throw std::runtime_error(
                std::format("getaddrinfo failed hostname:{} service:{} error {} message {}",
                            hostname, service, errno, ::strerror(errno)));
        }
        if (result == nullptr) [[unlikely]] {
            throw std::runtime_error(
                std::format("getaddrinfo returned successfully but with no results"));
        }

        ::memcpy(&addr_, result->ai_addr, result->ai_addrlen);
        ::freeaddrinfo(result);
    }

public:
    Address(const std::string_view &hostname, const std::string_view &service)
        : Address(hostname, service, AI_ALL) {}

    explicit Address(const std::string_view &ip, std::uint16_t port = 0)
        : Address(ip, std::to_string(port), AI_NUMERICHOST | AI_NUMERICSERV) {}

    Address(const sockaddr *addr, std::size_t len) { ::memcpy(&addr_, addr, len); };

    Address(const Address &other)
        : addr_(other.addr_) {}

    auto operator=(const Address &other) -> Address & {
        if (this == std::addressof(other)) {
            return *this;
        }
        addr_ = other.addr_;
    }

    [[nodiscard]]
    auto ip() const -> std::string {
        char buf[64];
        if (is_ipv4()) {
            auto addr4 = reinterpret_cast<const sockaddr_in *>(&addr_);
            ::inet_ntop(AF_INET, &addr4->sin_addr, buf, sizeof(buf));
        } else {
            auto addr6 = reinterpret_cast<const sockaddr_in6 *>(&addr_);
            ::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, sizeof(buf));
        }
        return buf;
    }

    [[nodiscard]]
    auto port() const -> std::uint16_t {
        if (is_ipv4()) {
            auto addr4 = reinterpret_cast<const sockaddr_in *>(&addr_);
            return ::ntohs(addr4->sin_port);
        } else {
            auto addr6 = reinterpret_cast<const sockaddr_in6 *>(&addr_);
            return ::ntohs(addr6->sin6_port);
        }
    }

    [[nodiscard]]
    auto to_string() const -> std::string {
        return ip() + " " + std::to_string(port());
    };

    [[nodiscard]]
    auto get_sockaddr() const -> const sockaddr * {
        return reinterpret_cast<const sockaddr *>(&addr_);
    }

    [[nodiscard]]
    auto get_sockaddr() -> sockaddr * {
        return reinterpret_cast<sockaddr *>(&addr_);
    }

    [[nodiscard]]
    auto get_length() const noexcept -> std::size_t {
        if (is_ipv4()) {
            return sizeof(sockaddr_in);
        } else {
            return sizeof(sockaddr_in6);
        }
    }

    [[nodiscard]]
    auto is_ipv4() const noexcept -> bool {
        return addr_.ss_family == AF_INET;
    }

    [[nodiscard]]
    auto is_ipv6() const noexcept -> bool {
        return addr_.ss_family == AF_INET6;
    }

    [[nodiscard]]
    auto family() const -> sa_family_t {
        return addr_.ss_family;
    }

private:
    sockaddr_storage addr_{};
};

using AddressRef = std::shared_ptr<Address>;

} // namespace zed::net