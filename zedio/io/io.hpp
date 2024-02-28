#pragma once

#include "zedio/async/coroutine/task.hpp"
#include "zedio/common/concepts.hpp"
#include "zedio/io/awaiter/accept.hpp"
#include "zedio/io/awaiter/close.hpp"
#include "zedio/io/awaiter/connect.hpp"
#include "zedio/io/awaiter/fsync.hpp"
#include "zedio/io/awaiter/openat.hpp"
#include "zedio/io/awaiter/read.hpp"
#include "zedio/io/awaiter/read_vectored.hpp"
#include "zedio/io/awaiter/recv.hpp"
#include "zedio/io/awaiter/send.hpp"
#include "zedio/io/awaiter/shutdown.hpp"
#include "zedio/io/awaiter/statx.hpp"
#include "zedio/io/awaiter/write.hpp"
#include "zedio/io/awaiter/write_vectored.hpp"
// Linux
#include <netdb.h>

namespace zedio::io {

class IO {
private:
    explicit IO(int fd)
        : fd_{fd} {}

public:
    ~IO() {
        if (fd_ >= 0) {
            // TODO register closer
            if (async_close(fd_) == false) {
                sync_close(fd_);
            }
            fd_ = -1;
        }
    }

    IO(IO &&other) noexcept
        : fd_{other.fd_} {
        other.fd_ = -1;
    }

    auto operator=(IO &&other) noexcept -> IO & {
        if (fd_ >= 0) {
            sync_close(fd_);
        }
        fd_ = other.fd_;
        other.fd_ = -1;
        return *this;
    }

    [[nodiscard]]
    auto close() noexcept {
        auto fd = fd_;
        fd_ = -1;
        return Close{fd};
    }

    [[nodiscard]]
    auto shutdown(Shutdown::How how) const noexcept {
        return Shutdown{fd_, static_cast<int>(how)};
    }

    [[nodiscard]]
    auto read(std::span<char> buf) const noexcept {
        return Read{fd_, buf, static_cast<std::size_t>(-1)};
    }

    // template <typename T>
    // [[nodiscard]]
    // auto read_type(T &buf) const noexcept {
    //     return this->read{std::span<T>(std::addressof(buf), 1)};
    // }

    template <typename... Ts>
    [[nodiscard]]
    auto read_vectored(Ts &...bufs) const noexcept {
        constexpr auto N = sizeof...(Ts);

        class Awaiter : public ReadVectored {
        public:
            Awaiter(int fd,Ts&...bufs)
                : ReadVectored{fd, iovecs_.data(),iovecs_.size(),static_cast<std::size_t>(-1)}
                , iovecs_{ iovec{
                  .iov_base = std::span<char>(bufs).data(),
                  .iov_len = std::span<char>(bufs).size_bytes(),
                }...} {}

        private:
            std::array<struct iovec, N> iovecs_;
        };
        return Awaiter{fd, bufs...};
    }

    [[nodiscard]]
    auto write(std::span<const char> buf) const noexcept {
        return Write{fd_, buf, static_cast<std::size_t>(-1)};
    }

    [[nodiscard]]
    auto write_all(std::span<const char> buf) const noexcept -> zedio::async::Task<Result<void>> {
        auto has_written_bytes{0uz};
        auto remaining_bytes{buf.size_bytes()};
        while (remaining_bytes > 0) {
            auto ret = co_await this->write({buf.data() + has_written_bytes, remaining_bytes});
            if (!ret) [[unlikely]] {
                co_return std::unexpected{ret.error()};
            }
            has_written_bytes += ret.value();
            remaining_bytes -= ret.value();
        }
        co_return Result<void>{};
    }

    template <class T>
    [[nodiscard]]
    auto read_to_end(T &buf) const noexcept -> zedio::async::Task<Result<void>> {
        auto offset = buf.size();
        {
            auto ret = co_await this->metadata();
            if (!ret) {
                co_return std::unexpected{ret.error()};
            }
            buf.resize(offset + ret.value().stx_size);
        }
        auto span = std::span<char>{buf}.subspan(offset);
        auto ret = Result<std::size_t>{};
        while (true) {
            ret = co_await this->read(span);
            if (!ret) [[unlikely]] {
                co_return std::unexpected{ret.error()};
            }
            if (ret.value() == 0) {
                break;
            }
            span = span.subspan(ret.value());
            if (span.empty()) {
                break;
            }
        }
        co_return Result<void>{};
    }

    template <typename... Ts>
    [[nodiscard]]
    auto write_vectored(Ts &...bufs) const noexcept {
        constexpr auto N = sizeof...(Ts);

        class Awaiter : public WriteVectored {
        public:
            Awaiter(int fd,Ts&...bufs)
                : WriteVectored{fd, &iovecs_, N, static_cast<std::size_t>(-1)}
                , iovecs_{ iovec{
                  .iov_base = std::span<char>(bufs).data(),
                  .iov_len = std::span<char>(bufs).size_bytes(),
                }...} {}

        private:
            std::array<struct iovec, N> iovecs_;
        };
        return Awaiter{fd_, bufs...};
    }

    template <typename T>
    [[nodiscard]]
    auto send(std::span<const T> buf) const noexcept {
        return Send{fd_, buf, MSG_NOSIGNAL};
    }

    template <typename T, typename Addr>
        requires is_socket_address<Addr>
    [[nodiscard]]
    auto send_to(std::span<const T> buf, const Addr &addr) const noexcept {
        return SendTo(fd_, buf, MSG_NOSIGNAL, addr);
    }

    template <typename T>
    [[nodiscard]]
    auto recv(std::span<T> buf, int flags = 0) const noexcept {
        return Recv{fd_, buf, flags};
    }

    [[nodiscard]]
    auto fd() const noexcept {
        return fd_;
    }

    [[nodiscard]]
    auto take_fd() noexcept {
        auto ret = fd_;
        fd_ = -1;
        return ret;
    }

    template <typename Addr>
        requires is_socket_address<Addr>
    [[nodiscard]]
    auto connect(const Addr &addr) const noexcept {
        return Connect(fd_, addr.sockaddr(), addr.length());
    }

    template <typename Addr>
        requires is_socket_address<Addr>
    [[nodiscard]]
    auto bind(const Addr &addr) const noexcept -> Result<void> {
        if (::bind(fd_, addr.sockaddr(), addr.length()) == -1) [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        return {};
    }

    [[nodiscard]]
    auto listen(int n) const noexcept -> Result<void> {
        if (::listen(fd_, n) == -1) [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        return {};
    }

    [[nodiscard]]
    auto fsync(unsigned int fsync_flags) noexcept {
        return Fsync{fd_, fsync_flags};
    }

    [[nodiscard]]
    auto metadata() const noexcept {
        return StatxToMetadata{fd_};
    }

    template <typename Addr>
        requires is_socket_address<Addr>
    [[nodiscard]]
    auto local_addr() const noexcept -> Result<Addr> {
        struct sockaddr_storage addr {};
        socklen_t               len{sizeof(addr)};
        if (::getsockname(fd_, reinterpret_cast<struct sockaddr *>(&addr), &len) == -1)
            [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        return Addr{reinterpret_cast<struct sockaddr *>(&addr), len};
    }

    template <typename Addr>
        requires is_socket_address<Addr>
    [[nodiscard]]
    auto peer_addr() const noexcept -> Result<Addr> {
        struct sockaddr_storage addr {};
        socklen_t               len{sizeof(addr)};
        if (::getpeername(fd_, reinterpret_cast<struct sockaddr *>(&addr), &len) == -1)
            [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        return Addr{reinterpret_cast<struct sockaddr *>(&addr), len};
    }

    [[nodiscard]]
    auto set_reuseaddr(bool on) const noexcept -> Result<void> {
        auto optval{on ? 1 : 0};
        return set_sock_opt(SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    }

    [[nodiscard]]
    auto reuseaddr() const noexcept -> Result<bool> {
        auto optval{0};
        if (auto ret = get_sock_opt(SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)); ret)
            [[likely]] {
            return optval != 0;
        } else {
            return std::unexpected{ret.error()};
        }
    }

    [[nodiscard]]
    auto set_reuseport(bool on) const noexcept -> Result<void> {
        auto optval{on ? 1 : 0};
        return set_sock_opt(SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    }

    [[nodiscard]]
    auto reuseport() const noexcept -> Result<bool> {
        auto optval{0};
        if (auto ret = get_sock_opt(SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)); ret)
            [[likely]] {
            return optval != 0;
        } else {
            return std::unexpected{ret.error()};
        }
    }

    [[nodiscard]]
    auto set_ttl(uint32_t ttl) const noexcept -> Result<void> {
        return set_sock_opt(IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));
    }

    [[nodiscard]]
    auto ttl() const noexcept -> Result<uint32_t> {
        uint32_t optval{0};
        if (auto ret = get_sock_opt(IPPROTO_IP, IP_TTL, &optval, sizeof(optval)); ret) [[likely]] {
            return optval;
        } else {
            return std::unexpected{ret.error()};
        }
    }

    [[nodiscard]]
    auto set_linger(std::optional<std::chrono::seconds> duration) const noexcept -> Result<void> {
        struct linger lin {
            .l_onoff{0}, .l_linger{0},
        };
        if (duration.has_value()) {
            lin.l_onoff = 1;
            lin.l_linger = duration.value().count();
        }
        return set_sock_opt(SOL_SOCKET, SO_LINGER, &lin, sizeof(lin));
    }

    [[nodiscard]]
    auto linger() const noexcept -> Result<std::optional<std::chrono::seconds>> {
        struct linger lin;
        if (auto ret = get_sock_opt(SOL_SOCKET, SO_LINGER, &lin, sizeof(lin)); ret) [[likely]] {
            if (lin.l_onoff == 0) {
                return std::nullopt;
            } else {
                return std::chrono::seconds(lin.l_linger);
            }
        } else {
            return std::unexpected{ret.error()};
        }
    }

    [[nodiscard]]
    auto set_broadcast(bool on) const noexcept -> Result<void> {
        auto optval{on ? 1 : 0};
        return set_sock_opt(SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));
    }

    [[nodiscard]]
    auto broadcast() const noexcept -> Result<bool> {
        auto optval{0};
        if (auto ret = get_sock_opt(SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)); ret)
            [[likely]] {
            return optval != 0;
        } else {
            return std::unexpected{ret.error()};
        }
    }

    [[nodiscard]]
    auto set_keepalive(bool on) const noexcept {
        auto optval{on ? 1 : 0};
        return set_sock_opt(SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
    }

    [[nodiscard]]
    auto keepalive() const noexcept -> Result<bool> {
        auto optval{0};
        if (auto ret = get_sock_opt(SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)); ret)
            [[likely]] {
            return optval != 0;
        } else {
            return std::unexpected{ret.error()};
        }
    }

    [[nodiscard]]
    auto set_recv_buffer_size(int size) const noexcept {
        return set_sock_opt(SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
    }

    [[nodiscard]]
    auto recv_buffer_size() const noexcept -> Result<std::size_t> {
        auto size{0};
        if (auto ret = get_sock_opt(SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)); ret) [[likely]] {
            return static_cast<std::size_t>(size);
        } else {
            return std::unexpected{ret.error()};
        }
    }

    [[nodiscard]]
    auto set_send_buffer_size(int size) const noexcept {
        return set_sock_opt(SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
    }

    [[nodiscard]]
    auto send_buffer_size() const noexcept -> Result<std::size_t> {
        auto size{0};
        if (auto ret = get_sock_opt(SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)); ret) [[likely]] {
            return static_cast<std::size_t>(size);
        } else {
            return std::unexpected{ret.error()};
        }
    }

    [[nodiscard]]
    auto set_nodelay(bool on) const noexcept -> Result<void> {
        auto flags = ::fcntl(fd_, F_GETFL, 0);
        if (on) {
            flags |= O_NDELAY;
        } else {
            flags &= ~O_NDELAY;
        }
        if (::fcntl(fd_, F_SETFL, flags) == -1) [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        return {};
    }

    [[nodiscard]]
    auto nodelay() const noexcept -> Result<bool> {
        auto flags = ::fcntl(fd_, F_GETFL, 0);
        if (flags == -1) [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        return flags & O_NDELAY;
    }

    [[nodiscard]]
    auto set_nonblocking(bool status) const noexcept -> Result<void> {
        auto flags = ::fcntl(fd_, F_GETFL, 0);
        if (status) {
            flags |= O_NONBLOCK;
        } else {
            flags &= ~O_NONBLOCK;
        }
        if (::fcntl(fd_, F_SETFL, flags) == -1) [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        return {};
    }

    [[nodiscard]]
    auto nonblocking() const noexcept -> Result<bool> {
        auto flags = ::fcntl(fd_, F_GETFL, 0);
        if (flags == -1) [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        return flags & O_NONBLOCK;
    }

    [[nodiscard]]
    auto set_mark(uint32_t mark) const noexcept {
        return set_sock_opt(SOL_SOCKET, SO_MARK, &mark, sizeof(mark));
    }

    [[nodiscard]]
    auto set_passcred(bool on) const noexcept {
        int optval{on ? 1 : 0};
        return set_sock_opt(SOL_SOCKET, SO_PASSCRED, &optval, sizeof(optval));
    }

    [[nodiscard]]
    auto passcred() const noexcept -> Result<bool> {
        int optval{0};
        if (auto ret = get_sock_opt(SOL_SOCKET, SO_PASSCRED, &optval, sizeof(optval)); ret)
            [[likely]] {
            return optval != 0;
        } else {
            return std::unexpected{ret.error()};
        }
    }

private:
    [[nodiscard]]
    auto set_sock_opt(int level, int optname, const void *optval, socklen_t optlen) const noexcept
        -> Result<void> {
        if (::setsockopt(fd_, level, optname, optval, optlen) == -1) [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        return {};
    }

    [[nodiscard]]
    auto get_sock_opt(int level, int optname, void *optval, socklen_t optlen) const noexcept
        -> Result<void> {
        if (auto ret = ::getsockopt(fd_, level, optname, optval, &optlen); ret == -1) [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        return {};
    }

    static void sync_close(int fd) noexcept {
        int ret = 0;
        int cnt = 3;
        do {
            if (ret = ::close(fd); ret == 0) [[likely]] {
                return;
            }
        } while (ret == EINTR && cnt--);
        LOG_ERROR("sync close {} failed, error: {}", ret, strerror(errno));
    }

    static auto async_close(int fd) noexcept -> bool {
        auto sqe = detail::t_poller->get_sqe();
        if (sqe == nullptr) {
            LOG_WARN("async close fd failed, sqe is nullptr");
            return false;
        }
        io_uring_prep_close(sqe, fd);
        io_uring_sqe_set_data(sqe, nullptr);
        return true;
    }

public:
    [[nodiscard]]
    static auto socket(int domain, int type, int protocol) -> Result<IO> {
        if (auto fd = ::socket(domain, type | SOCK_NONBLOCK, protocol); fd != -1) [[likely]] {
            return IO{fd};
        } else {
            return std::unexpected{make_sys_error(errno)};
        }
    }

    template <class T>
    [[nodiscard]]
    static auto open(const std::string_view &path, int flags, mode_t mode) {
        class Awaiter : public OpenAt {
        public:
            Awaiter(int fd, const std::string_view &path, int flags, mode_t mode)
                : OpenAt{fd, path.data(), flags, mode} {}

            auto await_resume() const noexcept -> Result<T> {
                if (this->cb_.result_ >= 0) [[likely]] {
                    return T{IO{this->cb_.result_}};
                } else {
                    return std::unexpected{make_sys_error(-this->cb_.result_)};
                }
            }
        };
        return Awaiter{AT_FDCWD, path, flags, mode};
    }

    [[nodiscard]]
    static auto from_fd(int fd) -> IO {
        return IO{fd};
    }

private:
    int fd_;
};

} // namespace zedio::io
