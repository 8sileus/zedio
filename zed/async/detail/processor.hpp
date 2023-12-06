#pragma once

#include "async/detail/io_awaiter.hpp"
#include "async/detail/timer.hpp"
#include "async/detail/waker.hpp"
#include "common/config.hpp"
#include "common/util/thread.hpp"
#include "log.hpp"
// C
#include <cassert>
#include <cstring>
//  C++
#include <atomic>
#include <functional>

namespace zed::async::detail {

class Processor;

thread_local Processor *t_processor{nullptr};

class Processor {
public:
    Processor()
        : tid_{current_thread::get_tid()} {
        assert(t_processor == nullptr);
        t_processor = this;
        if (auto ret = io_uring_queue_init(config::IOURING_QUEUE_SIZE, this->get_uring(), 0);
            ret != 0) [[unlikely]] {
            LOG_FATAL("Call io_uring_queue_init failed, error : {}.", strerror(-ret));
            throw std::runtime_error(
                std::format("Call io_uring_queue_init failed, error: {}.", strerror(-ret)));
        }
        LOG_TRACE("Processor {} has constructed.", this->tid_);
    }

    ~Processor() {
        if (this->running_) {
            stop();
        }
        t_processor = nullptr;
        io_uring_queue_exit(this->get_uring());
        LOG_TRACE("Processor {} has destructed.", this->tid_);
    }

    void stop() {
        if (current_thread::get_tid() == this->tid_) {
            assert(this->running_);
            this->running_ = false;
        } else {
            this->post([this]() { this->stop(); });
        }
    }

    void start() {
        assert(!this->running_);

        LazyBaseIOAwaiter *awaiter{nullptr};
        io_uring_cqe      *cqe{nullptr};
        __kernel_timespec  ts{.tv_sec = config::IOURING_POLL_TIMEOUTOUT, .tv_nsec = 0};

        this->waker_.start();
        this->timer_.start();
        this->running_ = true;
        while (this->running_) {
            if (auto ret = io_uring_wait_cqe_timeout(this->get_uring(), &cqe, &ts); ret < 0)
                [[unlikely]] {
                if (-ret == ETIME) {
                    LOG_TRACE("Processor {} timer expired", this->tid_);
                    handle_waiting_awaiter();
                    continue;
                }
                LOG_ERROR("io_uring poll failed, error: {}.", strerror(-ret));
                break;
            }
            if (cqe) [[likely]] {
                awaiter
                    = reinterpret_cast<detail::LazyBaseIOAwaiter *>(io_uring_cqe_get_data64(cqe));
                awaiter->res_ = cqe->res;
                awaiter->handle_.resume();
                io_uring_cqe_seen(this->get_uring(), cqe);
            }
            handle_waiting_awaiter();
        }
    }

    void post(std::function<void()> &&task) {
        this->waker_.post(std::move(task));
        this->waker_.wake();
    }

    void post(Task<void> &&coroutine) {
        auto task = [handle = coroutine.take()] { handle.resume(); };
        this->post(std::move(task));
    }

    auto get_uring() -> io_uring * { return &this->uring_; }

    auto get_timer() -> Timer & { return this->timer_; }

    auto get_tid() const noexcept -> pid_t { return this->tid_; }

    void push_awaiter(LazyBaseIOAwaiter *awaiter) { this->waiting_awaiters_.push(awaiter); }

private:
    void handle_waiting_awaiter() {
        while (!this->waiting_awaiters_.empty()) {
            auto sqe = io_uring_get_sqe(this->get_uring());
            if (sqe == nullptr) [[unlikely]] {
                break;
            }
            auto awaiter = std::move(this->waiting_awaiters_.front());
            this->waiting_awaiters_.pop();
            awaiter->cb_(sqe);
            io_uring_sqe_set_data64(sqe, reinterpret_cast<unsigned long long>(awaiter));
            io_uring_submit(this->get_uring());
        }
    }

private:
    io_uring                        uring_{};
    Timer                           timer_{};
    Waker                           waker_{};
    std::queue<LazyBaseIOAwaiter *> waiting_awaiters_{};
    pid_t                           tid_;
    bool                            running_{false};
};

} // namespace zed::async::detail
