#pragma once

#include "async/async_io.hpp"
#include "async/task.hpp"
#include "common/debug.hpp"
#include "common/util/noncopyable.hpp"
// Linux
#include <sys/eventfd.h>
// C
#include <cstring>
// C++
#include <format>
#include <functional>
#include <mutex>
#include <vector>

namespace zed::async::detail {

class Waker : util::Noncopyable {
public:
    Waker()
        : handle_{work()}
        , fd_{::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)} {
        if (this->fd_ < 0) [[unlikely]] {
            throw std::runtime_error(std::format("call eventfd failed, errorno: {} message: {}",
                                                 this->fd_, strerror(errno)));
        }
    }

    ~Waker() { ::close(this->fd_); }

    void run() {
        handle_.resume();
    }

    void wake_up() {
        // debug for 3 hours NOTE:must not be zero
        uint64_t buf{1};
        if (auto ret = ::write(this->fd_, &buf, sizeof(buf)); ret != sizeof(buf)) [[unlikely]] {
            LOG_ERROR("Waker write failed, error: {}.", strerror(errno));
        }
    }

private:
    auto work() -> Task<void> {
        uint64_t buf{0};
        while (true) {
            if (auto result = co_await async::Read<AL::privated>(this->fd_, &buf, sizeof(buf));
                !result.has_value()) [[unlikely]] {
                LOG_ERROR("Waker read failed, error: {}.", result.error().message());
            }
        }
    }

private:
    Task<void>                         handle_;
    int                                fd_;
};

} // namespace zed::async::detail
