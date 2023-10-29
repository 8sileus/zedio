#pragma once

#include "common/macros.hpp"
#include "net/address.hpp"
#include "net/tcp_stream.hpp"
//C
#include <cstring>
// C++
#include <optional>
#include <vector>

namespace zed::net
{

class TcpListener {
private:
    TcpListener(int fd, const Address &address)
        : fd_{fd}
        , address_{address} {
        LOG_DEBUG("Build a listener FD: {}, LOCAL_ADDRESS: {}", fd, address_.to_string());
    }

public:
    auto get_local_address() -> const Address & { return address_; }

    auto accept() -> async::Task<std::optional<TcpStream>> {
        struct sockaddr addr;
        socklen_t       len;
        ::memset(&addr, 0, sizeof(addr));
        int fd = co_await async::Accept(fd_, &addr, &len);
        if (fd < 0) [[unlikely]] {
            LOG_FD_SYSERR(fd_, accept, -fd);
            co_return std::nullopt;
        }
        Address peer_addr{&addr, len};
        LOG_DEBUG("{} accept a connection from {}", address_.to_string(), peer_addr.to_string());
        co_return TcpStream{fd};
    }

    void set_reuse_address(bool reuse_address) {
        int optval = static_cast<int>(reuse_address);
        if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) != 0) [[unlikely]] {
            LOG_FD_SYSERR(fd_, setsockopt, errno);
        }
    }

public:
    static auto bind(const Address &address) -> std::optional<TcpListener> {
        auto fd = ::socket(address.get_family(), SOCK_STREAM, 0);
        if (fd == -1) [[unlikely]] {
            return std::nullopt;
        }
        if (::bind(fd, address.get_sockaddr(), address.get_length()) != 0) [[unlikely]] {
            return std::nullopt;
        }
        if (::listen(fd, SOMAXCONN) != 0) [[unlikely]] {
            return std::nullopt;
        }
        return TcpListener{fd, address};
    }

    static auto bind(const std::vector<Address> &addresses) -> std::optional<TcpListener> {
        for (const auto &address : addresses) {
            if (auto tcp_listener = TcpListener::bind(address); tcp_listener.has_value()) {
                return tcp_listener;
            }
        }
    }

private:
    int fd_;
    Address address_;
};

} // namespace zed::net
