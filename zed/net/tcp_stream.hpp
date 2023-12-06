#pragma once

#include "async.hpp"
#include "common/macros.hpp"
#include "common/util/noncopyable.hpp"
#include "net/address.hpp"
#include "net/socket.hpp"
// C++
#include <chrono>
#include <optional>
// Linux
#include <sys/socket.h>

namespace zed::net {

namespace detail {
    struct AcceptStream;

} // namespace detail

class TcpStream : util::Noncopyable {
    friend struct detail::AcceptStream;

public:
    enum class ShutdownAction { READ = SHUT_RD, WRITE = SHUT_WR, ALL = SHUT_RDWR };

private:
    explicit TcpStream(int fd)
        : fd_{fd} {}

public:
    ~TcpStream() {
        if (fd_ >= 0) [[likely]] {
            ::close(fd_);
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
    auto shutdown(ShutdownAction action) -> async::Shutdown {
        return async::Shutdown(fd_, static_cast<int>(action));
    }

    [[nodiscard]]
    auto read(void *buf, std::size_t len) -> async::Read {
        return async::Read(fd_, buf, len);
    }

    [[nodiscard]]
    auto readv(const iovec *iovecs, int nr_vecs) -> async::Readv {
        return async::Readv(fd_, iovecs, nr_vecs);
    }

    [[nodiscard]]
    auto write(const void *buf, std::size_t len) -> async::Write {
        return async::Write(fd_, buf, len);
    }

    [[nodiscard]]
    auto set_read_timeout(const std::chrono::microseconds &timeout = 0ms) -> std::error_code {
        struct timeval tv {
            .tv_sec = timeout.count() / 1000'000, .tv_usec = timeout.count() % 1000'000
        };
        return set_sock_opt(this->fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }

    [[nodiscard]]
    auto set_write_time(const std::chrono::milliseconds &timeout = 0ms) -> std::error_code {
        struct timeval tv {
            .tv_sec = timeout.count() / 1000'000, .tv_usec = timeout.count() % 1000'000
        };
        return set_sock_opt(this->fd_, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    }

    [[nodiscard]]
    auto get_read_timeout() -> std::chrono::milliseconds {
        struct timeval tv;
        ::memset(&tv, 0, sizeof(tv));
        socklen_t len = sizeof(tv);
        auto      ret = ::getsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, &len);
        if (ret != 0 || len != sizeof(tv)) [[unlikely]] {
            LOG_FD_SYSERR(fd_, getsockopt, errno);
        }
        return std::chrono::milliseconds{tv.tv_sec * 1000 + tv.tv_usec};
    }

    [[nodiscard]]
    auto get_write_timeout() -> std::chrono::milliseconds {
        struct timeval tv;
        ::memset(&tv, 0, sizeof(tv));
        socklen_t len = sizeof(tv);
        auto      ret = ::getsockopt(fd_, SOL_SOCKET, SO_SNDTIMEO, &tv, &len);
        if (ret != 0 || len != sizeof(tv)) [[unlikely]] {
            LOG_FD_SYSERR(fd_, getsockopt, errno);
        }
        return std::chrono::milliseconds{tv.tv_sec * 1000 + tv.tv_usec};
    }

    [[nodiscard]]
    auto get_local_address() -> std::expected<Address, std::error_code> {
        return net::get_local_address(this->fd_);
    }

    [[nodiscard]]
    auto get_peer_address() -> std::expected<Address, std::error_code> {
        return net::get_peer_address(this->fd_);
    }

    [[nodiscard]]
    auto get_fd() const -> int {
        return fd_;
    }

    [[nodiscard]]
    auto set_nodelay(bool need_delay) -> std::error_code {
        auto flags = ::fcntl(fd_, F_GETFL, 0);
        if (need_delay) {
            flags |= O_NDELAY;
        } else {
            flags &= ~O_NDELAY;
        }
        if (::fcntl(fd_, F_SETFL, flags) == -1) [[unlikely]] {
            return std::error_code{errno, std::system_category()};
        }
        return std::error_code{};
    }

    [[nodiscard]]
    auto is_nodelay() -> bool {
        auto flags = ::fcntl(fd_, F_GETFL, 0);
        return flags & O_NDELAY;
    }

    [[nodiscard]]
    auto set_nonblocking(bool nonblocking) -> std::error_code {
        auto flags = ::fcntl(fd_, F_GETFL, 0);
        if (nonblocking) {
            flags |= O_NONBLOCK;
        } else {
            flags &= ~O_NONBLOCK;
        }
        if (::fcntl(fd_, F_SETFL, flags) == -1) {
            return std::error_code{errno, std::system_category()};
        }
        return std::error_code{};
    }

    [[nodiscard]]
    auto is_nonblocking() -> bool {
        auto flags = ::fcntl(fd_, F_GETFL, 0);
        return flags & O_NONBLOCK;
    }

public:
    [[nodiscard]]
    static auto connect(const Address &address)
        -> async::Task<std::expected<TcpStream, std::error_code>> {
        int fd = ::socket(address.get_family(), SOCK_STREAM, 0);
        if (fd < 0) [[unlikely]] {
            co_return std::unexpected{
                std::error_code{errno, std::system_category()}
            };
        }
        auto ex = co_await async::Connect(fd, address.get_sockaddr(), address.get_length());
        if (!ex.has_value()) [[unlikely]] {
            co_return std::unexpected{ex.error()};
        }
        LOG_DEBUG("Build a client {{address: {},fd: {}}}", address.to_string(), fd);
        co_return TcpStream{fd};
    };

private:
    int fd_;
};

} // namespace zed::net
