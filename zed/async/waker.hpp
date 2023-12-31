#pragma once

#include "zed/async/awaiter_io.hpp"
#include "zed/async/task.hpp"
#include "zed/common/debug.hpp"
#include "zed/common/util/noncopyable.hpp"
// Linux
#include <sys/eventfd.h>
// C
#include <cstring>
// C++
#include <format>
#include <mutex>

namespace zed::async::detail {

class Waker : util::Noncopyable {
public:
    Waker()
        : loop_{loop()}
        , fd_{::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)}
        , idx_{t_poller->register_file(fd_).value()} {
        if (this->fd_ < 0) [[unlikely]] {
            throw std::runtime_error(std::format("call eventfd failed, errorno: {} message: {}",
                                                 this->fd_, strerror(errno)));
        }
        loop_.resume();
    }

    ~Waker() {
        // t_poller->unregister_file(idx_);
        ::close(this->fd_);
    }

    void wake_up() {
        // debug for 3 hours NOTE:must not be zero
        constexpr uint64_t buf{1};
        std::lock_guard    lock{mutex_};
        if (auto ret = ::write(this->fd_, &buf, sizeof(buf)); ret != sizeof(buf)) [[unlikely]] {
            LOG_ERROR("Waker write failed, error: {}.", strerror(errno));
        }
    }

    [[nodiscard]]
    auto fd() const -> int {
        return fd_;
    }

private:
    auto loop() -> Task<void> {
        uint64_t buf{0};
        while (true) {
            if (auto result
                = co_await ReadAwaiter<OPFlag::Registered>(this->idx_, &buf, sizeof(buf), 0);
                !result.has_value()) [[unlikely]] {
                LOG_ERROR("Waker read failed, error: {}.", result.error().message());
            }
        }
    }

private:
    Task<void> loop_;
    std::mutex mutex_;
    int        fd_;
    int        idx_;
};

} // namespace zed::async::detail
