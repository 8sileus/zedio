#include "address.hpp"
#include "log/log.hpp"

#include <algorithm>
#include <format>
#include <type_traits>

#include <arpa/inet.h>


namespace zed::net {

constexpr Address::Address(const std::string_view& hostname, const std::string_view& service, bool ipv6) {
    // AI_NUMERICHOST prohibit resolve host
    // AI_NUMERICSERV prohibit resolve service
    addrinfo  hints{.ai_flags{AI_NUMERICHOST | AI_NUMERICSERV}, .ai_family{ipv6 ? AF_INET6 : AF_INET}};
    addrinfo* result{nullptr};

    if (::getaddrinfo(hostname.data(), service.data(), &hints, &result) != 0) [[unlikely]] {
        std::runtime_error(std::format("getaddrinfo failed hostname:{} service:{} error message{}", hostname, service,
                                       ::strerror(errno)));
    }
    if (result == nullptr) [[unlikely]] {
        std::runtime_error(std::format("getaddrinfo returned successfully but with no results"));
    }

    *this = Address(result->ai_addr, result->ai_addrlen);
    ::freeaddrinfo(result);
}

explicit constexpr Address::Address(const sockaddr* addr, std::size_t size) {
    if(size==sizeof(sockaddr_in)){
        m_addr = *reinterpret_cast<const sockaddr_in*>(addr);
    } else if (size == sizeof(sockaddr_in6)) {
        m_addr = *reinterpret_cast<const sockaddr_in6*>(addr);
    } else {
        std::runtime_error("size is not equal to sockaddr_in6 or sockaddr_in ");
    }
}

}

}  // namespace zed::net