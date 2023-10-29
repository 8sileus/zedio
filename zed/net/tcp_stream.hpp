#pragma once

#include "async.hpp"
#include "common/macros.hpp"
#include "common/util/noncopyable.hpp"
//C++
#include <chrono>
#include <optional>
//Linux
#include <sys/socket.h>

namespace zed::net
{

class TcpListener;

class TcpStream : public util::Noncopyable {
private:
    friend class TcpListener;

    explicit TcpStream(int fd)
        : fd_{fd} {}

public:
    enum class ShutdownAction { READ, WRITE, READ_WRITE };

public:
    TcpStream(TcpStream &&other)
        : fd_(other.fd_) {
        other.fd_ = -1;
    }

    auto operator=(TcpStream &&other) -> TcpStream & {
        fd_ = other.fd_;
        other.fd_ = -1;
        return *this;
    }

    ~TcpStream() {
        if (fd_ != -1) {
            ::close(fd_);
        }
    }


    void close() {
        if (::close(fd_) != 0) [[unlikely]] {
            LOG_FD_SYSERR(fd_, close, errno);
        }
    }

    void shutdown(ShutdownAction action) {
        if (::shutdown(fd_, static_cast<int>(action)) != 0) [[unlikely]] {
            LOG_FD_SYSERR(fd_, shutdown, errno);
        }
    }

    // TODO finish
    [[nodiscard]]
    auto read(std::vector<char> &buf) {
        return async::Read(fd_, buf.data(), buf.size());
    }

    // TODO finish
    [[nodiscard]]
    auto write(const std::vector<char> &buf) {
        return async::Write(fd_, buf.data(), buf.size());
    }

    void set_read_timeout(const std::chrono::milliseconds &timeout = 0ms) {
        struct timeval tv {
            .tv_sec = timeout.count() / 1000, .tv_usec = timeout.count() % 1000
        };
        if (::setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0) [[unlikely]] {
            LOG_FD_SYSERR(fd_,setsockopt, errno);
        }
    }

    [[nodiscard]]
    auto get_read_timeout() -> std::chrono::milliseconds {
        struct timeval tv;
        ::memset(&tv, 0, sizeof(tv));
        socklen_t len = sizeof(tv);
        auto ret = ::getsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, &len);
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
            LOG_FD_SYSERR(fd_,getsockopt, errno);
        }
        return std::chrono::milliseconds{tv.tv_sec * 1000 + tv.tv_usec};
    }

    [[nodiscard]]
    auto get_local_address() -> Address {
        struct sockaddr_storage addr;
        socklen_t               len;
        ::getsockname(fd_, reinterpret_cast<struct sockaddr *>(&addr), &len);
        return Address(reinterpret_cast<struct sockaddr *>(&addr), len);
    }

    [[nodiscard]]
    auto get_peer_address() -> Address {
        struct sockaddr_storage addr;
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
        LOG_DEBUG("Build a stream FD: {}, PEER_ADDRESS: {}", fd, address.to_string());
        co_return TcpStream{fd};
    };

private:
    int fd_;
};

} // namespace zed::net
