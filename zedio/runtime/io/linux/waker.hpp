#pragma once

#include "zedio/runtime/io/linux/poller.hpp"
// Linux
#include <sys/eventfd.h>
#include <unistd.h>

namespace zedio::runtime::detail {

class Waker {
public:
    Waker(Poller &poller)
        : poller{poller}
        , fd_{::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)} {
        if (fd_ < 0) [[unlikely]] {
            throw std::runtime_error(
                std::format("call eventfd failed, error: {} - {}", fd_, strerror(errno)));
        }
    }

    ~Waker() {
        ::close(fd_);
    }

public:
    void wake_up() {
        static constexpr uint64_t buf{1};
        if (auto ret = ::write(this->fd_, &buf, sizeof(buf)); ret != sizeof(buf)) [[unlikely]] {
            LOG_ERROR("Waker write failed, error: {}.", strerror(errno));
        }
    }

    void turn_on() {
        if (flag_ != 0) {
            flag_ = 0;
            auto sqe = poller.get_sqe();
            assert(sqe != nullptr);
            io_uring_prep_read(sqe, fd_, &flag_, sizeof(flag_), 0);
            io_uring_sqe_set_data(sqe, nullptr);
        }
    }

private:
    uint64_t flag_{1};
    Poller  &poller;
    int      fd_;
};

} // namespace zedio::runtime::detail