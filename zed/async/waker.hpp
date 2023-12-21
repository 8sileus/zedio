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
        : fd_{::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)}
        , handle_{work()} {
        if (this->fd_ < 0) [[unlikely]] {
            throw std::runtime_error(std::format("call eventfd failed, errorno: {} message: {}",
                                                 this->fd_, strerror(errno)));
        }
        handle_.resume();
    }

    ~Waker() { ::close(this->fd_); }

    void wake_up() {
        char buf[8];
        if (auto ret = ::write(this->fd_, buf, sizeof(buf)); ret != sizeof(buf)) [[unlikely]] {
            LOG_ERROR("Waker write failed, error: {}.", strerror(ret));
        }
    }

private:
    auto work() -> Task<void> {
        char buf[8];
        while (true) {
            if (auto result = co_await async::Read<AL::privated>(this->fd_, buf, sizeof(buf));
                !result.has_value()) [[unlikely]] {
                LOG_ERROR("Waker read failed, error: {}.", result.error().message());
            }
        }
    }

private:
    Task<void>                         handle_{};
    int                                fd_;
};

} // namespace zed::async::detail
