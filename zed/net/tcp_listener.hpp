#pragma once

#include "common/macros.hpp"
#include "common/util/noncopyable.hpp"
#include "net/address.hpp"
#include "net/tcp_stream.hpp"
// C
#include <cstring>
// C++
#include <expected>
#include <vector>

namespace zed::net {

namespace detail {
    struct AcceptStream : public async::Accept {
        AcceptStream(int fd, sockaddr *addr, socklen_t *addrlen, int flags = 0)
            : Accept{fd, addr, addrlen, flags} {}

        auto await_resume() const noexcept -> std::expected<TcpStream, std::error_code> {
            if (LazyBaseIOAwaiter::res_ >= 0) [[likely]] {
                return TcpStream{res_};
            }
            return std::unexpected{
                std::error_code{-res_, std::system_category()}
            };
        }
    };
} // namespace detail

class TcpListener : util::Noncopyable {
private:
    TcpListener(int fd, const Address &address)
        : fd_{fd}
        , address_{address} {}

public:

    ~TcpListener() {
        if (this->fd_ >= 0) [[likely]] {
            ::close(this->fd_);
        }
        this->fd_ = -1;
    }

    TcpListener(TcpListener &&other)
        : fd_(other.fd_)
        , address_(std::move(other.address_)) {
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
        address_ = std::move(other.address_);
        other.fd_ = -1;
        return *this;
    }

    [[nodiscard]]
    auto accept() -> detail::AcceptStream {
        return detail::AcceptStream{this->fd_, nullptr, nullptr};
    }

    [[nodiscard]]
    auto address() const -> const Address & {
        return this->address_;
    }

    [[nodiscard]]
    auto fd() const noexcept -> int {
        return this->fd_;
    }

    [[nodiscard]]
    auto set_reuse_address(bool status) -> std::error_code {
        auto optval = status ? 1 : 0;
        return set_sock_opt(this->fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    }

public:
    [[nodiscard]]
    static auto bind(const Address &address) -> std::expected<TcpListener, std::error_code> {
        auto fd = ::socket(address.get_family(), SOCK_STREAM, 0);
        if (fd == -1) [[unlikely]] {
            return std::unexpected{
                std::error_code{errno, std::system_category()}
            };
        }
        if (::bind(fd, address.get_sockaddr(), address.get_length()) != 0) [[unlikely]] {
            ::close(fd);
              return std::unexpected{
                std::error_code{errno, std::system_category()}
            };
        }
        if (::listen(fd, SOMAXCONN) != 0) [[unlikely]] {
            ::close(fd);
            return std::unexpected{
                std::error_code{errno, std::system_category()}
            };
        }
        return TcpListener{fd, address};
    }

    [[nodiscard]]
    static auto bind(const std::vector<Address> &addresses)
        -> std::expected<TcpListener, std::error_code> {
        for (const auto &address : addresses) {
            if (auto result = TcpListener::bind(address); result.has_value()) {
                return result;
            }
        }
        // TODO return error
    }

private:
    int     fd_;
    Address address_;
};

} // namespace zed::net
