#pragma once

#include "zedio/common/debug.hpp"
#include "zedio/runtime/config.hpp"
// C
#include <cstring>
// C++
#include <chrono>
#include <format>
// Linux
#include <liburing.h>

namespace zedio::runtime::detail {
class IORing;

inline thread_local IORing *t_ring{nullptr};

class IORing {
public:
    IORing(const Config &config)
        : num_max_weak_submissions_{config.num_weak_submissions_} {
        if (auto ret = io_uring_queue_init(config.ring_entries_, &ring_, config.io_uring_flags_);
            ret < 0) [[unlikely]] {
            throw std::runtime_error(
                std::format("Call io_uring_queue_init failed, error: {}.", strerror(-ret)));
        }
        assert(t_ring == nullptr);
        t_ring = this;
    }

    ~IORing() {
        io_uring_queue_exit(&ring_);
        t_ring = nullptr;
    }

public:
    [[nodiscard]]
    auto ring() -> struct io_uring * {
        return &ring_;
    }

    [[nodiscard]]
    auto get_sqe() -> struct io_uring_sqe * {
        return io_uring_get_sqe(&ring_);
    }

    [[nodiscard]]
    auto peek_batch(std::span<io_uring_cqe *> cqes) -> std::size_t {
        return io_uring_peek_batch_cqe(&ring_, cqes.data(), cqes.size());
    }

    void wait(std::optional<time_t> timeout) {
        io_uring_cqe *cqe{nullptr};
        if (timeout) {
            // LOG_DEBUG("wait for {}", timeout.value());
            struct __kernel_timespec ts {
                .tv_sec = timeout.value() / 1000, .tv_nsec = timeout.value() % 1000 * 1000'000,
            };
            if (auto ret = io_uring_wait_cqe_timeout(&ring_, &cqe, &ts); ret != 0 && ret != -ETIME)
                [[unlikely]] {
                LOG_ERROR("io_uring_wait_cqe_timeout failed, error: {}", strerror(-ret));
            }
        } else {
            if (auto ret = io_uring_wait_cqe(&ring_, &cqe); ret != 0) [[unlikely]] {
                LOG_ERROR("io_uring_wait_cqe failed, error: {}", strerror(-ret));
            }
        }
    }

    void consume(std::size_t cnt) {
        io_uring_cq_advance(&ring_, cnt);
    }

    void submit() {
        num_cur_weak_submissions_ += 1;
        if (num_cur_weak_submissions_ == num_max_weak_submissions_) {
            force_submit();
        }
    }

    void force_submit() {
        num_max_weak_submissions_ = 0;
        if (auto ret = io_uring_submit(&ring_); ret < 0) [[unlikely]] {
            LOG_ERROR("submit sqes failed, {}", strerror(-ret));
        }
    }

private:
    struct io_uring ring_ {};
    uint32_t        num_cur_weak_submissions_{0};
    uint32_t        num_max_weak_submissions_;
};

} // namespace zedio::runtime::detail
