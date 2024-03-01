#pragma once

#include "zedio/async/coroutine/task.hpp"
#include "zedio/io/awaiter/accept.hpp"
#include "zedio/io/awaiter/cancel.hpp"
#include "zedio/io/awaiter/close.hpp"
#include "zedio/io/awaiter/cmd_sock.hpp"
#include "zedio/io/awaiter/connect.hpp"
#include "zedio/io/awaiter/fsync.hpp"
#include "zedio/io/awaiter/link.hpp"
#include "zedio/io/awaiter/mkdir.hpp"
#include "zedio/io/awaiter/open.hpp"
#include "zedio/io/awaiter/read.hpp"
#include "zedio/io/awaiter/read_vectored.hpp"
#include "zedio/io/awaiter/recv.hpp"
#include "zedio/io/awaiter/rename.hpp"
#include "zedio/io/awaiter/send.hpp"
#include "zedio/io/awaiter/shutdown.hpp"
#include "zedio/io/awaiter/socket.hpp"
#include "zedio/io/awaiter/statx.hpp"
#include "zedio/io/awaiter/tee.hpp"
#include "zedio/io/awaiter/unlink.hpp"
#include "zedio/io/awaiter/waitid.hpp"
#include "zedio/io/awaiter/write.hpp"
#include "zedio/io/awaiter/write_vectored.hpp"
// Linux
#include <netdb.h>

namespace zedio::io {

class IO {
protected:
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

    [[REMEMBER_CO_AWAIT]]
    auto close() noexcept {
        auto fd = fd_;
        fd_ = -1;
        return Close{fd};
    }

    [[REMEMBER_CO_AWAIT]]
    auto read(std::span<char> buf) const noexcept {
        return Read{fd_,
                    buf.data(),
                    static_cast<unsigned int>(buf.size_bytes()),
                    static_cast<std::size_t>(-1)};
    }

    template <typename... Ts>
    [[REMEMBER_CO_AWAIT]]
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

    [[REMEMBER_CO_AWAIT]]
    auto write(std::span<const char> buf) const noexcept {
        return Write{fd_,
                     buf.data(),
                     static_cast<unsigned int>(buf.size_bytes()),
                     static_cast<std::size_t>(-1)};
    }

    [[REMEMBER_CO_AWAIT]]
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

    template <typename... Ts>
    [[REMEMBER_CO_AWAIT]]
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

    [[nodiscard]]
    auto set_nodelay(bool on) noexcept -> Result<void> {
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

private:
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
        auto sqe = detail::t_driver->get_sqe();
        if (sqe == nullptr) {
            LOG_WARN("async close fd failed, sqe is nullptr");
            return false;
        }
        io_uring_prep_close(sqe, fd);
        io_uring_sqe_set_data(sqe, nullptr);
        return true;
    }

protected:
    int fd_;
};

} // namespace zedio::io
