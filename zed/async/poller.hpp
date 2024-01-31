#pragma once

#include "zed/async/awaiter_data.hpp"
#include "zed/async/queue.hpp"
#include "zed/async/task.hpp"
#include "zed/common/config.hpp"
#include "zed/common/debug.hpp"
#include "zed/common/error.hpp"
#include "zed/common/util/noncopyable.hpp"
// C
#include <cassert>
#include <cstring>
// C++
#include <chrono>
#include <expected>
#include <format>
#include <numeric>
#include <queue>
// Linux
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
        if (auto ret = io_uring_register_ring_fd(&ring_); ret < 0) [[unlikely]] {
            throw std::runtime_error(
                std::format("Call io_uring_register_ring_fd failed, error: {}.", strerror(-ret)));
        }
        if (auto ret = io_uring_register_files_sparse(&ring_, zed::config::FIXED_FILES_NUM);
            ret < 0) [[unlikely]] {
            throw std::runtime_error(
                std::format("Call io_uring_register_files failed, error: {}.", strerror(-ret)));
        }
        // if (auto ret = io_uring_register_buffers(&ring_, buffers_.data(), buffers_.size()); ret <
        // 0)
        //     [[unlikely]] {
        //     throw std::runtime_error(
        //         std::format("Call io_uring_register_buffers failed, error: {}.",
        //         strerror(-ret)));
        // }
        // std::iota(buffer_indexes_.begin(), buffer_indexes_.end(), 0);
        std::iota(file_indexes_.begin(), file_indexes_.end(), 0);
        assert(t_poller == nullptr);
        t_poller = this;
    }

    ~Poller() {
        // TODO: why block on io_uring_unregister_files
        // if (auto ret = io_uring_unregister_files(&ring_); ret < 0) [[unlikely]] {
        //     LOG_DEBUG("Call io_uring_unregister_files failed, error: {}", strerror(-ret));
        // }

        // if (auto ret = io_uring_unregister_buffers(&ring_); ret < 0) [[unlikely]] {
        //     LOG_ERROR("Call io_uring_unregister_buffers failed, error: {}", strerror(-ret));
        // }
        if (auto ret = io_uring_unregister_ring_fd(&ring_); ret < 0) [[unlikely]] {
            LOG_ERROR("Call io_uring_unregister_ring_fd failed, error: {}", strerror(-ret));
        }
        io_uring_queue_exit(&ring_);
    }

    // Current worker thread will be blocked on io_uring_wait_cqe
    // until other worker wakes up it or a I/O event completes
    auto wait(std::optional<std::coroutine_handle<>> &run_next) {
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
            data->set_result(cqe->res);
            if (data->is_distributable()) {
                run_next = data->handle_;
            } else {
                data->handle_.resume();
            }
        }
        io_uring_cqe_seen(&ring_, cqe);
    }

    [[nodiscard]]
    auto poll(LocalQueue &queue, GlobalQueue &global_queue) -> bool {
        std::array<io_uring_cqe *, zed::config::LOCAL_QUEUE_CAPACITY> cqes;
        auto cnt = io_uring_peek_batch_cqe(&ring_, cqes.data(), zed::config::LOCAL_QUEUE_CAPACITY);
        if (cnt == 0) [[unlikely]] {
            return false;
        }
        for (auto i = 0uz; i < cnt; i += 1) {
            auto data = reinterpret_cast<BaseIOAwaiterData *>(cqes[i]->user_data);
            if (data != nullptr) [[likely]] {
                data->set_result(cqes[i]->res);
                if (data->is_distributable()) {
                    queue.push_back_or_overflow(std::move(data->handle_), global_queue);
                } else {
                    data->handle_.resume();
                }
            }
            io_uring_cqe_seen(&ring_, cqes[i]);
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
        assert(!file_indexes_.empty());
        auto index = file_indexes_.back();
        file_indexes_.pop_back();
        if (auto ret = io_uring_register_files_update(&ring_, index, &fd, 1); ret < 0)
            [[unlikely]] {
            return std::unexpected{make_sys_error(-ret)};
        }
        return index;
    }

    void unregister_file(std::size_t index) {
        // LOG_DEBUG("unregister {}", index);
        file_indexes_.push_back(index);
        auto fd = -1;
        if (auto ret = io_uring_register_files_update(&ring_, index, &fd, 1); ret < 0)
            [[unlikely]] {
            LOG_ERROR("Unregister file offset {} failed ,error {}", index, strerror(-ret));
        };
    }

    // [[nodiscard]]
    // auto register_buffer(void *buffer, std::size_t len) -> std::size_t {
    //     assert(!buffer_indexes_.empty());
    //     auto idx = buffer_indexes_.back();
    //     buffer_indexes_.pop_back();
    //     buffers_[idx].iov_base = buffer;
    //     buffers_[idx].iov_len = len;
    //     io_uring_register_buffers_update_tag(&ring_, idx, &buffers_[idx], nullptr, 1);
    //     return idx;
    // }

    // void unregister_buffer(std::size_t idx) {
    //     buffer_indexes_.push_back(idx);
    //     buffers_[idx].iov_base = nullptr;
    //     buffers_[idx].iov_len = 0;
    //     io_uring_register_buffers_update_tag(&ring_, idx, &buffers_[idx], nullptr, 1);
    // }

private:
    io_uring                 ring_{};
    std::vector<std::size_t> file_indexes_{std::vector<std::size_t>(zed::config::FIXED_FILES_NUM)};
    // std::array<int, 10>                                  files_{};
    // std::array<struct iovec, 10> buffers_;
    // std::vector<std::size_t>     buffer_indexes_{10};
    // std::queue<detail::BaseIOAwaiter *> waiting_awaiters_;
};

} // namespace zed::async::detail
