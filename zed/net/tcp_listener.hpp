#pragma once

#include "common/macros.hpp"
#include "common/util/noncopyable.hpp"
#include "net/address.hpp"
#include "net/tcp_stream.hpp"
// C
#include <cstring>
// C++
#include <optional>
#include <variant>
#include <vector>

namespace zed::net {

namespace detail {
    struct AcceptStream : public async::Accept {
        AcceptStream(int fd, sockaddr *addr, socklen_t *addrlen, int flags = 0)
            : Accept{fd, addr, addrlen, flags} {}

        auto await_resume() const noexcept -> std::optional<TcpStream> {
            if (LazyBaseIOAwaiter::res_ >= 0) [[likely]] {
                return TcpStream(res_);
            }
            return std::nullopt;
        }
    };
} // namespace detail

class TcpListener : util::Noncopyable {
public:
    TcpListener() = default;

    ~TcpListener() {
        if (this->fd_ >= 0) [[likely]] {
            ::close(this->fd_);
        }
        this->fd_ = -1;
    }

    TcpListener(TcpListener &&other)
        : fd_(other.fd_) {
        other.fd_ = -1;
    }

    auto operator=(TcpListener &&other) -> TcpListener & {
        if (this == std::addressof(other)) [[unlikely]] {
            return *this;
        }
        if (this->fd_ >= 0) {
            ::close(this->fd_);
        }
        this->fd_ = other.fd_;
        other.fd_ = -1;
        return *this;
    }

    [[nodiscard]]
    auto bind(const Address &address) -> std::error_code {
        auto fd = ::socket(address.get_family(), SOCK_STREAM, 0);
        if (fd == -1) [[unlikely]] {
            return std::error_code{errno, std::system_category()};
        }
        if (::bind(fd, address.get_sockaddr(), address.get_length()) != 0) [[unlikely]] {
            ::close(fd);
            return std::error_code{errno, std::system_category()};
        }
        if (::listen(fd, SOMAXCONN) != 0) [[unlikely]] {
            ::close(fd);
            return std::error_code{errno, std::system_category()};
        }
        this->fd_ = fd;
        return std::error_code{};
    }

    [[nodiscard]]
    auto accept() -> detail::AcceptStream {
        return detail::AcceptStream{this->fd_, nullptr, nullptr};
    }

    [[nodiscard]]
    auto get_address() const -> Address {
        return get_local_address(this->fd_);
    }

    [[nodiscard]]
    auto get_fd() const noexcept -> int {
        return this->fd_;
    }

    [[nodiscard]]
    auto set_reuse_address(bool status) -> std::error_code {
        auto optval = status ? 1 : 0;
        if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) != 0)
            [[unlikely]] {
            return std::error_code{errno, std::system_category()};
        }
        return std::error_code{};
    }

private:
    int fd_{-1};
};

} // namespace zed::net
