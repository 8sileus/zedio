#pragma once

#include "zedio/common/error.hpp"
// C
#include <cinttypes>
// C++
#include <format>
#include <string>
// Linux
#include <arpa/inet.h>
#include <netdb.h>

namespace zedio::socket::net {

class Ipv4Addr {
public:
    Ipv4Addr(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
        : ip_{static_cast<uint32_t>(a) | static_cast<uint32_t>(b) << 8
              | static_cast<uint32_t>(c) << 16 | static_cast<uint32_t>(d) << 24} {}

    Ipv4Addr(uint32_t ip)
        : ip_{ip} {}

    [[nodiscard]]
    auto addr() const noexcept -> uint32_t {
        return ip_;
    }

    [[nodiscard]]
    auto to_string() const -> std::string {
        char buf[16];
        ::inet_ntop(AF_INET, &ip_, buf, sizeof(buf));
        return buf;
    }

public:
    [[nodiscard]]
    static auto parse(std::string_view ip) -> Result<Ipv4Addr> {
        uint32_t addr;
        if (::inet_pton(AF_INET, ip.data(), &addr) != 1) {
            return std::unexpected{make_sys_error(errno)};
        }
        return Ipv4Addr{addr};
    }

public:
    [[nodiscard]]
    friend auto
    operator==(const Ipv4Addr &left, const Ipv4Addr &right) -> bool {
        return left.ip_ == right.ip_;
    }

private:
    uint32_t ip_;
};

class Ipv6Addr {
public:
    Ipv6Addr(uint16_t a,
             uint16_t b,
             uint16_t c,
             uint16_t d,
             uint16_t e,
             uint16_t f,
             uint16_t g,
             uint16_t h) {
        ip_.__in6_u.__u6_addr16[0] = ::htons(a);
        ip_.__in6_u.__u6_addr16[1] = ::htons(b);
        ip_.__in6_u.__u6_addr16[2] = ::htons(c);
        ip_.__in6_u.__u6_addr16[3] = ::htons(d);
        ip_.__in6_u.__u6_addr16[4] = ::htons(e);
        ip_.__in6_u.__u6_addr16[5] = ::htons(f);
        ip_.__in6_u.__u6_addr16[6] = ::htons(g);
        ip_.__in6_u.__u6_addr16[7] = ::htons(h);
    }

    Ipv6Addr(in6_addr ip)
        : ip_{ip} {}

    [[nodiscard]]
    auto addr() const noexcept -> const in6_addr & {
        return ip_;
    }

    [[nodiscard]]
    auto to_string() const -> std::string {
        char buf[64];
        ::inet_ntop(AF_INET6, &ip_, buf, sizeof(buf));
        return buf;
    }

public:
    [[nodiscard]]
    static auto parse(std::string_view ip) -> Result<Ipv6Addr> {
        in6_addr addr;
        if (::inet_pton(AF_INET6, ip.data(), &addr) != 1) [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        return Ipv6Addr{addr};
    }

public:
    [[nodiscard]]
    friend auto
    operator==(const Ipv6Addr &left, const Ipv6Addr &right) -> bool {
        return ::memcmp(&left.ip_, &right.ip_, sizeof(left.ip_)) == 0;
    }

private:
    in6_addr ip_;
};

class SocketAddr {
public:
    SocketAddr() = default;

public:
    SocketAddr(const sockaddr *addr, std::size_t len) {
        std::memcpy(&addr_, addr, len);
    }

    SocketAddr(Ipv4Addr ip, uint16_t port) {
        addr_.in4.sin_family = AF_INET;
        addr_.in4.sin_port = ::htons(port);
        addr_.in4.sin_addr.s_addr = ip.addr();
    }

    SocketAddr(const Ipv6Addr &ip, uint16_t port) {
        addr_.in6.sin6_family = AF_INET6;
        addr_.in6.sin6_port = ::htons(port);
        addr_.in6.sin6_addr = ip.addr();
    }

    [[nodiscard]]
    auto ip() const noexcept -> std::variant<Ipv4Addr, Ipv6Addr> {
        if (is_ipv4()) {
            return Ipv4Addr{addr_.in4.sin_addr.s_addr};
        } else {
            return Ipv6Addr{addr_.in6.sin6_addr};
        }
    }

    void set_ip(Ipv4Addr ip) {
        addr_.in4.sin_addr.s_addr = ip.addr();
    }

    void set_ip(const Ipv6Addr &ip) {
        addr_.in6.sin6_addr = ip.addr();
    }

    [[nodiscard]]
    auto port() const -> uint16_t {
        return ::ntohs(addr_.in6.sin6_port);
    }

    void set_port(uint16_t port) {
        addr_.in6.sin6_port = ::htons(port);
    }

    [[nodiscard]]
    auto to_string() const -> std::string {
        char buf[128];
        if (is_ipv4()) {
            ::inet_ntop(AF_INET, &addr_.in4.sin_addr, buf, sizeof(buf) - 1);
            return std::string{buf} + ":" + std::to_string(this->port());
        } else {
            buf[0] = '[';
            ::inet_ntop(AF_INET6, &addr_.in6.sin6_addr, buf + 1, sizeof(buf) - 1);
            return std::string{buf} + "]:" + std::to_string(this->port());
        }
    }

    [[nodiscard]]
    auto is_ipv4() const noexcept -> bool {
        return addr_.in4.sin_family == AF_INET;
    }

    [[nodiscard]]
    auto is_ipv6() const noexcept -> bool {
        return addr_.in6.sin6_family == AF_INET6;
    }

    [[nodiscard]]
    auto family() const noexcept -> int {
        return addr_.in6.sin6_family;
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
        if (is_ipv4()) {
            return sizeof(sockaddr_in);
        } else {
            return sizeof(sockaddr_in6);
        }
    }

public:
    [[nodiscard]]
    static auto parse(std::string_view host_name, uint16_t port) -> Result<SocketAddr> {
        addrinfo hints{};
        hints.ai_flags = AI_NUMERICSERV;
        addrinfo *result{nullptr};
        if (::getaddrinfo(host_name.data(), std::to_string(port).data(), &hints, &result) != 0)
            [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        if (result == nullptr) [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        SocketAddr sock{result->ai_addr, result->ai_addrlen};
        ::freeaddrinfo(result);
        return sock;
    }

private:
    union {
        sockaddr_in  in4;
        sockaddr_in6 in6;
    } addr_;
};

} // namespace zedio::socket::net

namespace std {

template <>
class formatter<zedio::socket::net::Ipv4Addr> {
public:
    constexpr auto parse(format_parse_context &context) {
        auto it{context.begin()};
        auto end{context.end()};
        if (it == end || *it == '}') {
            return it;
        }
        ++it;
        if (it != end && *it != '}') {
            throw format_error("Invalid format specifier for Ipv4Addr");
        }
        return it;
    }

    auto format(zedio::socket::net::Ipv4Addr addr, auto &context) const noexcept {
        return format_to(context.out(), "{}", addr.to_string());
    }
};

template <>
class formatter<zedio::socket::net::Ipv6Addr> {
public:
    constexpr auto parse(format_parse_context &context) {
        auto it{context.begin()};
        auto end{context.end()};
        if (it == end || *it == '}') {
            return it;
        }
        ++it;
        if (it != end && *it != '}') {
            throw format_error("Invalid format specifier for Ipv6Addr");
        }
        return it;
    }

    auto format(const zedio::socket::net::Ipv6Addr &addr, auto &context) const noexcept {
        return format_to(context.out(), "{}", addr.to_string());
    }
};

template <>
class formatter<zedio::socket::net::SocketAddr> {
public:
    constexpr auto parse(format_parse_context &context) {
        auto it{context.begin()};
        auto end{context.end()};
        if (it == end || *it == '}') {
            return it;
        }
        ++it;
        if (it != end && *it != '}') {
            throw format_error("Invalid format specifier for SocketAddr");
        }
        return it;
    }

    auto format(const zedio::socket::net::SocketAddr &addr, auto &context) const noexcept {
        return format_to(context.out(), "{}", addr.to_string());
    }
};

} // namespace std
