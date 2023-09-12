#pragma once

#include "async/async_io.hpp"
#include "util/noncopyable.hpp"
// Linux
#include <sys/eventfd.h>
// C
#include <cstring>
//  C++
#include <format>

namespace zed::async::detail {

class Waker : util::Noncopyable {
public:
    Waker()
        : fd_(::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)) {
        if (fd_ < 0) [[unlikely]] {
            // TODO log error
            throw std::runtime_error(
                std::format("call eventfd failed, errorno: {} message: {}", fd_, strerror(errno)));
        }
    }

    ~Waker() { ::close(fd_); }

    void run() {
        task_ = [this]() -> Task<void> {
            char buf[8];
            while (true) {
                if (co_await async::read(fd_, buf, sizeof(buf), 0, false) != sizeof(buf))
                    [[unlikely]] {
                    // TODO log error
                }
            }
        }();
        task_.resume();
    }

    void wake() {
        char buf[8];
        if (::write(fd_, buf, sizeof(buf)) != sizeof(buf)) [[unlikely]] {
            // TODO log error
        }
    }

    auto fd() const -> int { return fd_; }

private:
    Task<void> task_;
    int        fd_{-1};
};

} // namespace zed::async::detail
