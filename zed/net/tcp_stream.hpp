#pragma once

#include "async.hpp"
#include "common/macros.hpp"
#include "common/util/noncopyable.hpp"
// C++
#include <chrono>
#include <optional>
// Linux
#include <sys/socket.h>

namespace zed::net {

class TcpStream : util::Noncopyable {
public:
    enum class ShutdownAction { READ = SHUT_RD, WRITE = SHUT_WR, READ_WRITE = SHUT_RDWR };

public:
    explicit TcpStream(int fd)
        : fd_{fd} {}

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

    void set_read_timeout(const std::chrono::milliseconds &timeout = 0ms) {
        struct timeval tv {
            .tv_sec = timeout.count() / 1000, .tv_usec = timeout.count() % 1000
        };
        if (::setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0) [[unlikely]] {
            LOG_FD_SYSERR(fd_, setsockopt, errno);
        }
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

    void set_write_time(const std::chrono::milliseconds &timeout = 0ms) {
        struct timeval tv {
            .tv_sec = timeout.count() / 1000, .tv_usec = timeout.count() % 1000
        };
        if (::setsockopt(fd_, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) != 0) [[unlikely]] {
            LOG_FD_SYSERR(fd_, setsockopt, errno);
        }
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
    auto get_local_address() -> Address {
        struct sockaddr_storage addr;
        ::memset(&addr, 0, sizeof(addr));
        socklen_t len;
        ::getsockname(fd_, reinterpret_cast<struct sockaddr *>(&addr), &len);
        return Address(reinterpret_cast<struct sockaddr *>(&addr), len);
    }

    [[nodiscard]]
    auto get_peer_address() -> Address {
        struct sockaddr_storage addr;
        ::memset(&addr, 0, sizeof(addr));
        socklen_t               len;
        ::getpeername(fd_, reinterpret_cast<struct sockaddr *>(&addr), &len);
        return Address(reinterpret_cast<struct sockaddr *>(&addr), len);
    }

    void set_nodelay(bool nodelay) {
        auto flags = ::fcntl(fd_, F_GETFL, 0);
        if (nodelay) {
            flags |= O_NDELAY;
        } else {
            flags &= ~O_NDELAY;
        }
        ::fcntl(fd_, F_SETFL, flags);
    }

    [[nodiscard]]
    auto is_nodelay() -> bool {
        auto flags = ::fcntl(fd_, F_GETFL, 0);
        return flags & O_NDELAY;
    }

    // void set_nonblocking(bool nonblocking) {
    //     auto flags = ::fcntl(fd_, F_GETFL, 0);
    //     if (nonblocking) {
    //         flags |= O_NONBLOCK;
    //     } else {
    //         flags &= ~O_NONBLOCK;
    //     }
    //     ::fcntl(fd_, F_SETFL, flags);
    // }

    // auto is_nonblocking() -> bool{
    //     auto flags = ::fcntl(fd_, F_GETFL, 0);
    //     return flags & O_NONBLOCK;
    // }

public:
    [[nodiscard]]
    static auto connect(const Address &address) -> async::Task<std::optional<TcpStream>> {
        int fd = ::socket(address.get_family(), SOCK_STREAM, 0);
        if (fd < 0) [[unlikely]] {
            co_return std::nullopt;
        }
        if (co_await async::Connect(fd, address.get_sockaddr(), address.get_length()) < 0)
            [[unlikely]] {
            co_return std::nullopt;
        }
        LOG_DEBUG("Build a connection [ PEER_ADDR: {}, FD: {} ]", address.to_string(), fd);
        co_return TcpStream{fd};
    };

private:
    int fd_;
};

} // namespace zed::net
