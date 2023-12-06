#pragma once

#include "async/async_io.hpp"
#include "async/task.hpp"
#include "common/util/noncopyable.hpp"
#include "log.hpp"
// Linux
#include <sys/eventfd.h>
// C
#include <cstring>
// C++
#include <format>
#include <functional>

namespace zed::async::detail {

class Waker : util::Noncopyable {
public:
    Waker()
        : fd_(::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)) {
        if (fd_ < 0) [[unlikely]] {
            throw std::runtime_error(
                std::format("call eventfd failed, errorno: {} message: {}", fd_, strerror(errno)));
        }
    }

    ~Waker() { ::close(fd_); }

    void start() {
        task_ = work();
        task_.resume();
    }

    void wake() {
        char buf[8];
        if (auto n = ::write(fd_, buf, sizeof(buf)); n != sizeof(buf)) [[unlikely]] {
            log::zed_logger.error("write {}/{} bytes ", n, sizeof(buf));
        }
    }

    auto fd() const -> int { return fd_; }

private:
    auto work() -> Task<void> {
        char buf[8];
        while (true) {
            if (auto n = co_await async::Read(fd_, buf, sizeof(buf), 0); n != sizeof(buf))
                [[unlikely]] {
                log::zed_logger.error("waker read {}/{} bytes ", n, sizeof(buf));
            }
        }
    }

private:
    Task<void> task_{};
    int        fd_{-1};
};

} // namespace zed::async::detail
