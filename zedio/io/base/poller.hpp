#pragma once

#include "zedio/common/debug.hpp"
#include "zedio/io/base/callback.hpp"
#include "zedio/io/base/config.hpp"
#include "zedio/runtime/queue.hpp"
// C
#include <cstring>
// C++
#include <chrono>
#include <expected>
#include <format>
#include <functional>
#include <numeric>
#include <queue>
// Linux
#include <liburing.h>
namespace zedio::io::detail {

class Poller;

thread_local Poller *t_poller{nullptr};

class Poller : util::Noncopyable {
public:
    Poller(const Config &config) {
        if (auto ret = io_uring_queue_init(config.ring_entries_, &ring_, config.io_uring_flags_);
            ret < 0) [[unlikely]] {
            throw std::runtime_error(
                std::format("Call io_uring_queue_init failed, error: {}.", strerror(-ret)));
        }
        assert(t_poller == nullptr);
        t_poller = this;
    }

    ~Poller() {
        io_uring_queue_exit(&ring_);
        t_poller = nullptr;
    }

    // Current worker thread will be blocked on io_uring_wait_cqe
    // until other worker wakes up it or a I/O event completes
    auto wait(std::optional<std::coroutine_handle<>> &run_next) {
        if (auto ret = submit(); ret < 0) {
            LOG_ERROR("sub mit failed, error: {}", strerror(-ret));
        }

        io_uring_cqe *cqe{nullptr};
        if (auto ret = io_uring_wait_cqe(&ring_, &cqe); ret != 0) [[unlikely]] {
            LOG_DEBUG("io_uring_wait_cqe failed {}", strerror(-ret));
            return;
        }
        if (cqe == nullptr) {
            return;
        }
        auto cb = reinterpret_cast<Callback *>(cqe->user_data);
        if (cb != nullptr) [[likely]] {
            cb->result_ = cqe->res;
            if (cb->is_exclusive_) {
                cb->handle_.resume();
            } else {
                run_next = cb->handle_;
            }
        }
        io_uring_cqe_seen(&ring_, cqe);
    }

    [[nodiscard]]
    auto poll(runtime::detail::LocalQueue &queue) -> bool {
        constexpr const auto                      SIZE = Config::LOCAL_QUEUE_CAPACITY;
        std::array<io_uring_cqe *, SIZE>          cqes;
        std::array<std::coroutine_handle<>, SIZE> tasks;
        std::size_t                               shared_tasks = 0;
        std::size_t                               exclusive_tasks = SIZE;

        std::size_t cnt = io_uring_peek_batch_cqe(&ring_, cqes.data(), queue.remaining_slots());
        if (cnt == 0) {
            return false;
        }
        for (auto i = 0uz; i < cnt; i += 1) {
            auto cb = reinterpret_cast<Callback *>(cqes[i]->user_data);
            if (cb != nullptr) [[likely]] {
                cb->result_ = cqes[i]->res;
                if (cb->is_exclusive_) {
                    tasks[--exclusive_tasks] = std::move(cb->handle_);
                } else {
                    tasks[shared_tasks++] = std::move(cb->handle_);
                }
            }
        }
        assert(shared_tasks <= exclusive_tasks);
        LOG_TRACE("poll {} tasks {{shared: {}, private: {}}}",
                  cnt,
                  shared_tasks,
                  SIZE - exclusive_tasks);
        io_uring_cq_advance(&ring_, cnt);
        if (shared_tasks > 0) {
            queue.push_batch(
                std::span<std::coroutine_handle<>>{tasks.begin(), tasks.begin() + shared_tasks},
                shared_tasks);
        }
        while (exclusive_tasks < SIZE) {
            tasks[exclusive_tasks++].resume();
        }
        return true;
    }

    [[nodiscard]]
    auto ring() -> io_uring * {
        return &ring_;
    }

    [[nodiscard]]
    auto get_sqe() -> io_uring_sqe * {
        return io_uring_get_sqe(&ring_);
    }

    [[nodiscard]]
    auto submit() -> int {
        return io_uring_submit(&ring_);
    }

    void push_waiting_coro(std::function<void(io_uring_sqe *sqe)> &&cb) {
        waiting_coros_.push_back(std::move(cb));
    }

private:
private:
    io_uring                                          ring_{};
    std::list<std::function<void(io_uring_sqe *sqe)>> waiting_coros_;
};

} // namespace zedio::io::detail