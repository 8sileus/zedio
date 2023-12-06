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
#include <mutex>
#include <vector>

namespace zed::async::detail {

class Waker : util::Noncopyable {
public:
    Waker()
        : fd_{::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)} {
        if (this->fd_ < 0) [[unlikely]] {
            throw std::runtime_error(std::format("call eventfd failed, errorno: {} message: {}",
                                                 this->fd_, strerror(errno)));
        }
    }

    ~Waker() { ::close(this->fd_); }

    void start() {
        this->handle_ = work();
        this->handle_.resume();
    }

    void post(std::function<void()> &&task) {
        std::lock_guard lock(tasks_mutex_);
        tasks_.push_back(std::move(task));
    }

    void wake() {
        char buf[8];
        if (auto ret = ::write(this->fd_, buf, sizeof(buf)); ret != sizeof(buf)) [[unlikely]] {
            LOG_ERROR("Waker write failed, error: {}.", strerror(ret));
        }
    }

private:
    auto work() -> Task<void> {
        char buf[8];
        while (true) {
            if (auto ret = co_await async::Read(this->fd_, buf, sizeof(buf), 0); ret != sizeof(buf))
                [[unlikely]] {
                LOG_ERROR("Waker read failed, error: {}.", strerror(-ret));
            }
            std::vector<std::function<void()>> tasks;
            {
                std::lock_guard lock(this->tasks_mutex_);
                this->tasks_.swap(tasks);
            }
            for (auto &task : tasks) {
                task();
            }
        }
    }

private:
    Task<void>                         handle_{};
    std::mutex                         tasks_mutex_;
    std::vector<std::function<void()>> tasks_;
    int                                fd_;
};

} // namespace zed::async::detail
