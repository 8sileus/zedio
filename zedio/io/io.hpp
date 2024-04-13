#pragma once

#include "zedio/async/coroutine/task.hpp"
#include "zedio/io/awaiter/accept.hpp"
#include "zedio/io/awaiter/cancel.hpp"
#include "zedio/io/awaiter/close.hpp"
#include "zedio/io/awaiter/cmd_sock.hpp"
#include "zedio/io/awaiter/connect.hpp"
#include "zedio/io/awaiter/fadvise.hpp"
#include "zedio/io/awaiter/fallocate.hpp"
#include "zedio/io/awaiter/fsetxattr.hpp"
#include "zedio/io/awaiter/fsync.hpp"
#include "zedio/io/awaiter/getxattr.hpp"
#include "zedio/io/awaiter/link.hpp"
#include "zedio/io/awaiter/mkdir.hpp"
#include "zedio/io/awaiter/open.hpp"
#include "zedio/io/awaiter/read.hpp"
#include "zedio/io/awaiter/readv.hpp"
#include "zedio/io/awaiter/recv.hpp"
#include "zedio/io/awaiter/recvfrom.hpp"
#include "zedio/io/awaiter/rename.hpp"
#include "zedio/io/awaiter/send.hpp"
#include "zedio/io/awaiter/sendto.hpp"
#include "zedio/io/awaiter/setxattr.hpp"
#include "zedio/io/awaiter/shutdown.hpp"
#include "zedio/io/awaiter/socket.hpp"
#include "zedio/io/awaiter/statx.hpp"
#include "zedio/io/awaiter/symlink.hpp"
#include "zedio/io/awaiter/tee.hpp"
#include "zedio/io/awaiter/unlink.hpp"
#include "zedio/io/awaiter/waitid.hpp"
#include "zedio/io/awaiter/write.hpp"
#include "zedio/io/awaiter/writev.hpp"

namespace zedio::io::detail {

class FD {
protected:
    explicit FD(int fd)
        : fd_{fd} {}

    ~FD() {
        if (fd_ >= 0) {
            do_close();
        }
    }

    FD(FD &&other) noexcept
        : fd_{other.fd_} {
        other.fd_ = -1;
    }

    auto operator=(FD &&other) noexcept -> FD & {
        if (fd_ >= 0) {
            do_close();
        }
        fd_ = other.fd_;
        other.fd_ = -1;
        return *this;
    }

    // Delete copy
    FD(const FD &other) noexcept = default;
    auto operator=(const FD &other) noexcept -> FD & = default;

public:
    [[REMEMBER_CO_AWAIT]]
    auto close() noexcept {
        auto fd = fd_;
        fd_ = -1;
        return Close{fd};
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
    void do_close() noexcept {
        auto sqe = runtime::detail::t_ring->get_sqe();
        if (sqe != nullptr) [[likely]] {
            // async close
            io_uring_prep_close(sqe, fd_);
            io_uring_sqe_set_data(sqe, nullptr);
        } else {
            // sync close
            for (auto i = 1; i <= 3; i += 1) {
                auto ret = ::close(fd_);
                if (ret == 0) [[likely]] {
                    break;
                } else {
                    LOG_ERROR("close {} failed, error: {}, times: {}", fd_, strerror(errno), i);
                }
            }
        }
        fd_ = -1;
    }

protected:
    int fd_;
};

} // namespace zedio::io::detail
