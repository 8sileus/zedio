#pragma once

#include "async/awaiter_data.hpp"
#include "async/queue.hpp"
#include "common/config.hpp"
#include "common/debug.hpp"
#include "common/util/noncopyable.hpp"
// C
#include <cassert>
#include <cstring>
// C++
#include <chrono>
#include <expected>
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
        if (auto ret = io_uring_queue_init(zed::config::IOURING_QUEUE_SIZE, &ring_, 0); ret < 0)
            [[unlikely]] {
            throw std::runtime_error(
                std::format("Call io_uring_queue_init failed, error: {}.", strerror(-ret)));
        }
        if (auto ret = io_uring_register_files(&ring_, files_.data(), files_.size()); ret < 0)
            [[unlikely]] {
            throw std::runtime_error(
                std::format("Call io_uring_register_files failed, error: {}.", strerror(-ret)));
        }

        assert(t_poller == nullptr);
        t_poller = this;
    }

    ~Poller() {
        io_uring_unregister_files(&ring_);
        io_uring_queue_exit(&ring_);
    }

    // Current worker thread will be blocked on io_uring_wait_cqe
    // until other worker wakes up it or a I/O event completes
    auto wait(LocalQueue &queue, GlobalQueue &global_queue) {
        // handle_waiting_awatiers();

        io_uring_cqe *cqe{nullptr};
        while (true) {
            if (auto ret = io_uring_wait_cqe(&ring_, &cqe); ret < 0) [[unlikely]] {
                LOG_DEBUG("io_uring_wait_cqe failed {}", strerror(-ret));
                break;
            }
            if (cqe != nullptr) {
                break;
            }
        }
        auto data = reinterpret_cast<BaseIOAwaiterData *>(io_uring_cqe_get_data64(cqe));
        if (data != nullptr) [[likely]] {
            data->result_ = cqe->res;
            if (data->is_distributable()) {
                queue.push_back_or_overflow(std::move(data->handle_), global_queue);
            } else {
                data->handle_.resume();
            }
        }
        io_uring_cqe_seen(&ring_, cqe);
    }

    [[nodiscard]]
    auto poll(LocalQueue &queue, GlobalQueue &global_queue) -> bool {
        io_uring_cqe     *cqe{nullptr};
        __kernel_timespec ts{.tv_sec{0}, .tv_nsec{0}};

        if (auto ret
            = io_uring_wait_cqes(&ring_, &cqe, zed::config::LOCAL_QUEUE_CAPACITY, &ts, nullptr);
            ret < 0) [[unlikely]] {
            if (ret != -ETIME) {
                LOG_ERROR("io_uring_wait_cqes failed error: {}", strerror(-ret));
            }
            return false;
        }

        unsigned    head;
        io_uring_for_each_cqe(&ring_, head, cqe) {
            auto data = reinterpret_cast<BaseIOAwaiterData *>(io_uring_cqe_get_data64(cqe));
            // if is a cancel cqe, data will be nullptr
            if (data != nullptr) {
                data->result_ = cqe->res;
                if (data->is_distributable()) {
                    queue.push_back_or_overflow(std::move(data->handle_), global_queue);
                } else {
                    data->handle_.resume();
                }
            }
            io_uring_cqe_seen(&ring_, cqe);
        }
        return true;
    }

    // void handle_waiting_awatiers() {
    //     while (!waiting_awaiters_.empty()) {
    //         auto sqe = io_uring_get_sqe(&ring_);
    //         if (sqe == nullptr) {
    //             break;
    //         }
    //         auto awaiter = waiting_awaiters_.front();
    //         waiting_awaiters_.pop();
    //         awaiter->cb_(sqe);
    //     }
    // }

    [[nodiscard]]
    auto ring() -> io_uring * {
        return &ring_;
    }

    [[nodiscard]]
    auto register_file(int fd) -> std::expected<int, std::error_code> {
        files_[offset_] = fd;
        if (auto ret = io_uring_register_files_update(&ring_, offset_, &files_[offset_], 1);
            ret < 0) [[unlikely]] {
            return std::unexpected{
                std::error_code{-ret, std::system_category()}
            };
        }
        return offset_++;
    }

    // void unregister_file(int offset) {
    //     int fd = -1;
    //     if (auto ret = io_uring_register_files_update(&ring_, offset, &fd, 1); ret < 0)
    //         [[unlikely]] {
    //         LOG_ERROR("Unregister file offset {} failed ,error {}", offset, strerror(-ret));
    //     };
    // }

private:
    io_uring                                             ring_{};
    std::array<int, zed::config::IOURING_QUEUE_SIZE>     files_{-1};
    int                                                  offset_{0};
    // std::queue<detail::BaseIOAwaiter *> waiting_awaiters_;
};

} // namespace zed::async::detail
