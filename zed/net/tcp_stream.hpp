#pragma once

#include "zed/async.hpp"
#include "zed/common/util/noncopyable.hpp"
#include "zed/net/socket.hpp"
#include "zed/net/socket_addr.hpp"
// C++
#include <chrono>
#include <optional>
// Linux
#include <sys/socket.h>

namespace zed::net {

namespace detail {
    struct AcceptStream;

} // namespace detail

/// shutdown option
enum class SHUTDOWN_OPTION : int {
    READ = SHUT_RD,
    WRITE = SHUT_WR,
    READ_WRITE = SHUT_RDWR,
};

class TcpStream : util::Noncopyable {
    friend struct detail::AcceptStream;

private:
    explicit TcpStream(int fd)
        : fd_{fd} {}

public:
    ~TcpStream() {
        if (fd_ >= 0) [[likely]] {
            async::spwan([](int fd) -> async::Task<void> { co_await async::close(fd); }(fd_));
        }
        fd_ = -1;
    }

    TcpStream(TcpStream &&other)
        : fd_(other.fd_) {
        other.fd_ = -1;
    }

    auto operator=(TcpStream &&other) -> TcpStream & {
        if (this == std::addressof(other)) [[unlikely]] {
            return *this;
        }
        if (fd_ >= 0) {
            ::close(fd_);
        }
        fd_ = other.fd_;
        other.fd_ = -1;
        return *this;
    }

    [[nodiscard]]
    auto shutdown(SHUTDOWN_OPTION opition) {
        return async::shutdown(fd_, static_cast<int>(opition));
    }

    // support ptr
    [[nodiscard]]
    auto read(void *buf, std::size_t len) {
        return async::recv(fd_, buf, len, 0);
    }

    // support arr&
    template <typename T, std::size_t len>
    [[nodiscard]]
    auto read(T (&buf)[len]) {
        return this->read(buf, len);
    }

    [[nodiscard]]
    auto readv(const iovec *iovecs, int nr_vecs) {
        return async::readv(fd_, iovecs, nr_vecs, 0);
    }

    // ptr
    [[nodiscard]]
    auto write(const void *buf, std::size_t len) {
        return async::send(fd_, buf, len, 0);
    }

    // str
    [[nodiscard]]
    auto write(const std::string_view &buf) {
        return this->write(buf.data(), buf.length());
    }

    [[nodiscard]]
    auto local_address() -> std::expected<SocketAddr, std::error_code> {
        return net::get_local_address(this->fd_);
    }

    [[nodiscard]]
    auto peer_address() -> std::expected<SocketAddr, std::error_code> {
        return net::get_peer_address(this->fd_);
    }

    [[nodiscard]]
    auto get_fd() const -> int {
        return fd_;
    }

    [[nodiscard]]
    auto set_nodelay(bool need_delay) -> std::expected<void, std::error_code> {
        auto flags = ::fcntl(fd_, F_GETFL, 0);
        if (need_delay) {
            flags |= O_NDELAY;
        } else {
            flags &= ~O_NDELAY;
        }
        if (::fcntl(fd_, F_SETFL, flags) == -1) [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
    }

    [[nodiscard]]
    auto nodelay() -> std::expected<bool, std::error_code> {
        auto flags = ::fcntl(fd_, F_GETFL, 0);
        if (flags == -1) [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        return flags & O_NDELAY;
    }

    [[nodiscard]]
    auto set_linger(std::optional<std::chrono::seconds> duration)
        -> std::expected<void, std::error_code> {
        struct linger lin {
            .l_onoff{0}, .l_linger{0},
        };
        if (duration.has_value()) {
            lin.l_onoff = 1;
            lin.l_linger = duration.value().count();
        }
        return set_sock_opt(fd_, SOL_SOCKET, SO_LINGER, &lin, sizeof(lin));
    }

    [[nodiscard]]
    auto linger() -> std::expected<std::optional<std::chrono::seconds>, std::error_code> {
        struct linger lin;
        auto          ret = get_sock_opt(fd_, SOL_SOCKET, SO_LINGER, &lin, sizeof(lin));
        if (ret.has_value()) [[likely]] {
            if (lin.l_onoff == 0) {
                return std::nullopt;
            } else {
                return std::chrono::seconds(lin.l_linger);
            }
        }
        return std::unexpected{ret.error()};
    }

    [[nodiscard]]
    auto set_ttl(uint32_t ttl) -> std::expected<void, std::error_code> {
        return set_sock_opt(fd_, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));
    }

    [[nodiscard]]
    auto ttl() -> std::expected<uint32_t, std::error_code> {
        uint32_t result = 0;
        auto     ret = get_sock_opt(fd_, IPPROTO_IP, IP_TTL, &result, sizeof(result));
        if (ret.has_value()) [[likely]] {
            return result;
        }
        return std::unexpected{ret.error()};
    }

public:
    [[nodiscard]]
    static auto connect(const SocketAddr &address)
        -> async::Task<std::expected<TcpStream, std::error_code>> {
        auto ret = co_await async::socket(address.family(), SOCK_STREAM | SOCK_NONBLOCK, 0, 0);
        if (!ret.has_value()) [[unlikely]] {
            co_return std::unexpected{ret.error()};
        }
        auto fd = ret.value();
        ret = co_await async::connect(fd, address.sockaddr(), address.length());
        if (!ret.has_value()) [[unlikely]] {
            ::close(fd);
            co_return std::unexpected{ret.error()};
        }
        LOG_DEBUG("Build a connection to {}, fd: {}", address.to_string(), fd);
        co_return TcpStream{fd};
    };

private:
    int fd_;
};

} // namespace zed::net
