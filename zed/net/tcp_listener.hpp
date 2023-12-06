#pragma once

#include "common/macros.hpp"
#include "common/util/noncopyable.hpp"
#include "net/address.hpp"
#include "net/tcp_stream.hpp"
// C
#include <cstring>
// C++
#include <optional>
#include <vector>

namespace zed::net {

class TcpListener :  util::Noncopyable {
private:
    TcpListener(int fd, const Address &address)
        : fd_{fd}
        , address_{address} {
        LOG_DEBUG("Build a listener [ LOCAL_ADDR: {}, FD:{} ]", address_.to_string(), fd);
    }

public:
    ~TcpListener(){
        if (fd_ >= 0) [[likely]] {
            ::close(fd_);
        }
        fd_ = -1;
    }

    TcpListener(TcpListener &&other)
        : fd_(other.fd_)
        , address_(other.address_) {
        other.fd_ = -1;
    }

    auto operator=(TcpListener &&other) -> TcpListener & {
        if (this == std::addressof(other)) [[unlikely]] {
            return *this;
        }
        if (fd_ >= 0) {
            ::close(fd_);
        }
        fd_ = other.fd_;
        other.fd_ = -1;
        address_ = other.address_;
        return *this;
    }

    [[nodiscard]]
    auto accept() -> async::Accept {
        return async::Accept{fd_, nullptr, nullptr};
    }

    void set_reuse_address(bool reuse_address) {
        int optval = static_cast<int>(reuse_address);
        if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) != 0)
            [[unlikely]] {
            LOG_FD_SYSERR(fd_, setsockopt, errno);
        }
    }

    [[nodiscard]]
    auto address() const -> const Address & {
        return address_;
    }

    [[nodiscard]]
    auto fd() const -> int {
        return fd_;
    }

public:
    [[nodiscard]]
    static auto bind(const Address &address) -> std::optional<TcpListener> {
        auto fd = ::socket(address.get_family(), SOCK_STREAM, 0);
        if (fd == -1) [[unlikely]] {
            LOG_SYSERR(socket, errno);
            return std::nullopt;
        }
        if (::bind(fd, address.get_sockaddr(), address.get_length()) != 0) [[unlikely]] {
            LOG_FD_SYSERR(fd, bind, errno);
            return std::nullopt;
        }
        if (::listen(fd, SOMAXCONN) != 0) [[unlikely]] {
            LOG_FD_SYSERR(fd, listen, errno);
            return std::nullopt;
        }
        return TcpListener{fd, address};
    }

    [[nodiscard]]
    static auto bind(const std::vector<Address> &addresses) -> std::optional<TcpListener> {
        for (const auto &address : addresses) {
            if (auto tcp_listener = TcpListener::bind(address); tcp_listener.has_value()) {
                return tcp_listener;
            }
        }
        return std::nullopt;
    }

private:
    int     fd_{-1};
    Address address_;
};

} // namespace zed::net
