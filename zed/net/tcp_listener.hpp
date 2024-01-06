#pragma once

#include "common/debug.hpp"
#include "common/util/noncopyable.hpp"
#include "net/socket_addr.hpp"
#include "net/tcp_stream.hpp"
// C
#include <cstring>
// C++
#include <expected>
#include <vector>

namespace zed::net {

namespace detail {
    struct AcceptStream : public async::detail::AcceptAwaiter<async::detail::OPFlag::Registered> {
        AcceptStream(int fd)
            : AcceptAwaiter{fd, reinterpret_cast<sockaddr *>(&addr), &addrlen} {}

        auto await_resume() const noexcept -> std::expected<TcpStream, std::error_code> {
            auto result = AcceptAwaiter::await_resume();
            if (result.has_value()) [[likely]] {
                return TcpStream(result.value());
            }
            return std::unexpected{result.error()};
        }
        sockaddr_in6 addr;
        socklen_t    addrlen;
    };
} // namespace detail

class TcpListener : util::Noncopyable {
private:
    TcpListener(int fd)
        : fd_{fd}
        , idx_{async::detail::t_poller->register_file(fd_).value()} {
        LOG_TRACE("Build a tcp listener fd {}", fd_);
    }

public:
    ~TcpListener() {
        if (this->fd_ >= 0) [[likely]] {
            async::spwan([](int fd) -> async::Task<void> { co_await async::close(fd); }(fd_));
        }
        this->fd_ = -1;
    }

    TcpListener(TcpListener &&other)
        : fd_{other.fd_}
        , idx_{other.idx_} {
        other.fd_ = -1;
        other.idx_ = -1;
    }

    auto operator=(TcpListener &&other) -> TcpListener & {
        if (this == std::addressof(other)) [[unlikely]] {
            return *this;
        }
        if (this->fd_ >= 0) {
            ::close(this->fd_);
        }
        this->fd_ = other.fd_;
        this->idx_ = other.idx_;
        other.fd_ = -1;
        other.idx_ = -1;
        return *this;
    }

    [[nodiscard]]
    auto accept() -> detail::AcceptStream {
        return detail::AcceptStream{this->idx_};
    }

    [[nodiscard]]
    auto local_address() const -> std::expected<SocketAddr, std::error_code> {
        return get_local_address(this->fd_);
    }

    [[nodiscard]]
    auto fd() const noexcept -> int {
        return this->fd_;
    }

    [[nodiscard]]
    auto set_reuse_address(bool status) -> std::expected<void, std::error_code> {
        auto optval = status ? 1 : 0;
        return set_sock_opt(this->fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    }

public:
    [[nodiscard]]
    static auto bind(const SocketAddr &addresses) -> std::expected<TcpListener, std::error_code> {
        auto fd = ::socket(addresses.family(), SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (fd == -1) [[unlikely]] {
            return std::unexpected{
                std::error_code{errno, std::system_category()}
            };
        }
        if (::bind(fd, addresses.sockaddr(), addresses.length()) != 0) [[unlikely]] {
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
        return TcpListener{fd};
    }

    [[nodiscard]]
    static auto bind(const std::vector<SocketAddr> &addresses)
        -> std::expected<TcpListener, std::error_code> {
        for (const auto &address : addresses) {
            if (auto result = TcpListener::bind(address); result.has_value()) {
                return result;
            }
        }
        // TODO return error
    }

private:
    int fd_;
    int idx_{-1};
};

} // namespace zed::net
