#pragma once

#include "async/inject.hpp"
#include "async/io_awaiter.hpp"
#include "async/queue.hpp"
#include "common/config.hpp"
#include "common/debug.hpp"
#include "common/util/noncopyable.hpp"
// C
#include <cassert>
#include <cstring>
// C++
#include <chrono>
#include <format>
#include <queue>
// Dependencies
#include <liburing.h>
namespace zed::async::detail {

class Poller;

thread_local Poller *t_poller{nullptr};

class Poller : util::Noncopyable {
public:
    Poller() {
        if (auto ret = io_uring_queue_init(zed::config::IOURING_QUEUE_SIZE, &ring_, 0); ret != 0)
            [[unlikely]] {
            throw std::runtime_error(
                std::format("Call io_uring_queue_init failed, error: {}.", strerror(-ret)));
        }
        assert(t_poller == nullptr);
        t_poller = this;
    }

    ~Poller() {
        io_uring_queue_exit(&ring_);
    }

    // Current worker thread will be blocked on io_uring_wait_cqe
    // until other worker wakes up it or a I/O event completes
    auto wait() {
        handle_waiting_awatiers();

        io_uring_cqe *cqe{nullptr};
        while (true) {
            if (auto err = io_uring_wait_cqe(&ring_, &cqe); err != 0) [[unlikely]] {
                LOG_DEBUG("failed {}", err);
            }
            if (cqe != nullptr) {
                break;
            }
        }
        auto awaiter = reinterpret_cast<detail::BaseIOAwaiter *>(io_uring_cqe_get_data64(cqe));
        awaiter->res_ = cqe->res;
        awaiter->handle_.resume();
        io_uring_cqe_seen(&ring_, cqe);
    }

    [[nodiscard]]
    auto poll(LocalQueue &queue, GlobalQueue &global_queue) -> bool {
        io_uring_cqe     *cqe{nullptr};
        __kernel_timespec ts{.tv_sec{0}, .tv_nsec{0}};

        if (auto res
            = io_uring_wait_cqes(&ring_, &cqe, zed::config::LOCAL_QUEUE_CAPACITY, &ts, nullptr);
            res < 0) [[unlikely]] {
            if (res != -ETIME) {
                LOG_ERROR("io_uring_wait_cqes failed error: {}", strerror(-res));
            }
            return false;
        }

        unsigned    head;
        std::size_t cnt = 0;
        io_uring_for_each_cqe(&ring_, head, cqe) {
            auto awaiter = reinterpret_cast<detail::BaseIOAwaiter *>(io_uring_cqe_get_data64(cqe));
            awaiter->res_ = cqe->res;
            if (awaiter->level_ == AL::privated) {
                awaiter->handle_.resume();
            } else {
                queue.push_back_or_overflow(std::move(awaiter->handle_), global_queue);
            }
            cnt += 1;
        }
        io_uring_cq_advance(&ring_, cnt);
        return true;
    }

    void handle_waiting_awatiers() {
        while (!waiting_awaiters_.empty()) {
            auto sqe = io_uring_get_sqe(&ring_);
            if (sqe == nullptr) {
                break;
            }
            auto awaiter = waiting_awaiters_.front();
            waiting_awaiters_.pop();
            awaiter->cb_(sqe);
        }
    }

    [[nodiscard]]
    auto ring() -> io_uring * {
        return &ring_;
    }

    void push_awaiter(detail::BaseIOAwaiter *awaiter) {
        waiting_awaiters_.push(awaiter);
    }

private:
    io_uring                            ring_{};
    std::queue<detail::BaseIOAwaiter *> waiting_awaiters_;
};

void BaseIOAwaiter::await_suspend(std::coroutine_handle<> handle) {
    handle_ = std::move(handle);
    auto sqe = io_uring_get_sqe(t_poller->ring());
    if (sqe == nullptr) [[unlikely]] {
        t_poller->push_awaiter(this);
    } else {
        cb_(sqe);
        io_uring_sqe_set_data64(sqe, reinterpret_cast<unsigned long long>(this));
        io_uring_submit(t_poller->ring());
    }
}

} // namespace zed::async::detail
