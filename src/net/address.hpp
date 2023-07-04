#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <format>
#include <string_view>
#include <variant>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

namespace zed {

enum class IPVersion {
    IPV4,
    IPV6,
};

constexpr auto IPVersionToString(IPVersion version) -> const char* {
    switch (version) {
        case IPVersion::IPV4:
            return "IPV4";
        case IPVersion::IPV6:
            return "IPV6";
        default:
            return "Unknown ip version";
    }
}

template <IPVersion version>
class Address {
private:
    constexpr Address(const std::string_view& hostname,
                      const std::string_view& service, int flags) {
        static_assert(version == IPVersion::IPV4 || version == IPVersion::IPV6);

        addrinfo hints{.ai_flags{flags}};

        if constexpr (version == IPVersion::IPV4) {
            hints.ai_family = AF_INET;
        } else {
            hints.ai_family = AF_INET6;
        }

        addrinfo* result{nullptr};
        if (::getaddrinfo(hostname.data(), service.data(), &hints, &result))
            [[unlikely]] {
            throw std::runtime_error(std::format(
                "getaddrinfo failed hostname:{} service:{} error {} message {}",
                hostname, service, errno, ::strerror(errno)));
        }
        if (result == nullptr) [[unlikely]] {
            throw std::runtime_error(std::format(
                "getaddrinfo returned successfully but with no results"));
        }

        ::memcpy(&m_addr4, result->ai_addr, result->ai_addrlen);
        ::freeaddrinfo(result);
    }

public:
    constexpr Address(const std::string_view& hostname,
                      const std::string_view& service)
        : Address(hostname, service, AI_ALL) {}

    explicit constexpr Address(const std::string_view& ip,
                               std::uint16_t           port = 0)
        : Address(ip, std::to_string(port), AI_NUMERICHOST | AI_NUMERICSERV) {}

    explicit constexpr Address(const sockaddr* addr, std::size_t len) {
        static_assert(version == IPVersion::IPV4 || version == IPVersion::IPV6);
        ::memcpy(&m_addr4, addr, len);
    };

    auto ip() const -> std::string {
        char buf[64]{};
        if constexpr (version == IPVersion::IPV4) {
            ::inet_ntop(AF_INET, &m_addr4.sin_addr, buf, sizeof(buf));
        } else if constexpr (version == IPVersion::IPV6) {
            ::inet_ntop(AF_INET6, &m_addr6.sin6_addr, buf, sizeof(buf));
        }
        return buf;
    }

    auto port() const -> std::uint16_t {
        if constexpr (version == IPVersion::IPV4) {
            return ::ntohs(m_addr4.sin_port);
        } else {
            return ::ntohs(m_addr6.sin6_port);
        }
    }

    auto ipPort() const -> std::pair<std::string, std::uint16_t> {
        return {ip(), port()};
    }

    auto toString() const -> std::string {
        return ip() + ":" + std::to_string(port());
    };

    auto addr() const -> const sockaddr* {
        return reinterpret_cast<const sockaddr*>(m_addr4);
    }

    auto addr() -> sockaddr* { return reinterpret_cast<sockaddr*>(&m_addr4); }

    consteval auto len() const -> std::size_t {
        if constexpr (version == IPVersion::IPV4) {
            return sizeof(sockaddr_in);
        }
        return sizeof(sockaddr_in6);
    }

    consteval auto isIpv4() const -> bool { return version == IPVersion::IPV4; }

    consteval auto isIpv6() const -> bool { return version == IPVersion::IPV6; }

private:
    union {
        sockaddr_in  m_addr4;
        sockaddr_in6 m_addr6;
    };
};

using Address4 = Address<IPVersion::IPV4>;
using Address6 = Address<IPVersion::IPV6>;

}  // namespace zed