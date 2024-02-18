#pragma once

#include "zedio/async/awaiter_data.hpp"
#include "zedio/async/config.hpp"
#include "zedio/async/queue.hpp"
#include "zedio/async/task.hpp"
#include "zedio/common/config.hpp"
#include "zedio/common/debug.hpp"
#include "zedio/common/error.hpp"
#include "zedio/common/util/noncopyable.hpp"
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
namespace zedio::async::detail {

class Poller;

thread_local Poller *t_poller{nullptr};

class Poller : util::Noncopyable {
public:
    Poller(const Config &config) {
        if (auto ret = io_uring_queue_init(config.ring_entries_, &ring_,
                                           config.io_uring_flags_);
            ret < 0) [[unlikely]] {
            throw std::runtime_error(std::format(
                "Call io_uring_queue_init failed, error: {}.", strerror(-ret)));
        }
        if (auto ret = io_uring_register_files_sparse(
                &ring_, zedio::config::FIXED_FILES_NUM);
            ret < 0) [[unlikely]] {
            throw std::runtime_error(
                std::format("Call io_uring_register_files failed, error: {}.",
                            strerror(-ret)));
        }
        std::iota(file_indexes_.begin(), file_indexes_.end(), 0);
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
        // handle_waiting_awatiers();

        io_uring_cqe *cqe{nullptr};
        if (auto ret = io_uring_wait_cqe(&ring_, &cqe); ret != 0) [[unlikely]] {
            LOG_DEBUG("io_uring_wait_cqe failed {}", strerror(-ret));
            return;
        }
        if (cqe == nullptr) {
            return;
        }
        auto data = reinterpret_cast<BaseIOAwaiterData *>(cqe->user_data);
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
    auto poll(LocalQueue &queue) -> bool {
        constexpr const auto SIZE = zedio::config::LOCAL_QUEUE_CAPACITY;
        std::array<io_uring_cqe *, SIZE>          cqes;
        std::array<std::coroutine_handle<>, SIZE> tasks;
        std::size_t                               distributable_tasks = 0;
        std::size_t                               exclusive_tasks = SIZE;

        std::size_t cnt = io_uring_peek_batch_cqe(&ring_, cqes.data(),
                                                  queue.remaining_slots());
        if (cnt == 0) {
            return false;
        }
        for (auto i = 0uz; i < cnt; i += 1) {
            auto data
                = reinterpret_cast<BaseIOAwaiterData *>(cqes[i]->user_data);
            if (data != nullptr) [[likely]] {
                data->set_result(cqes[i]->res);
                if (data->is_distributable()) {
                    tasks[distributable_tasks++] = std::move(data->handle_);
                } else {
                    tasks[--exclusive_tasks] = std::move(data->handle_);
                }
            }
        }
        assert(distributable_tasks <= exclusive_tasks);
        LOG_TRACE("poll {} tasks {{shared: {}, private: {}}}", cnt,
                  distributable_tasks, SIZE - exclusive_tasks);
        io_uring_cq_advance(&ring_, cnt);
        if (distributable_tasks > 0) {
            queue.push_batch(
                std::span<std::coroutine_handle<>>{
                    tasks.begin(), tasks.begin() + distributable_tasks},
                distributable_tasks);
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

    void submit() {
        if (auto ret = io_uring_submit(&ring_); ret < 0) [[unlikely]] {
            LOG_ERROR("{}", strerror(-ret));
        }
    }

    [[nodiscard]]
    auto register_file(int fd) -> std::expected<int, std::error_code> {
        assert(!file_indexes_.empty());
        auto index = file_indexes_.back();
        file_indexes_.pop_back();
        if (auto ret = io_uring_register_files_update(&ring_, index, &fd, 1);
            ret < 0) [[unlikely]] {
            return std::unexpected{make_sys_error(-ret)};
        }
        return index;
    }

    void unregister_file(std::size_t index) {
        // LOG_DEBUG("unregister {}", index);
        file_indexes_.push_back(index);
        auto fd = -1;
        if (auto ret = io_uring_register_files_update(&ring_, index, &fd, 1);
            ret < 0) [[unlikely]] {
            LOG_ERROR("Unregister file offset {} failed ,error {}", index,
                      strerror(-ret));
        };
    }

private:
    io_uring                 ring_{};
    std::vector<std::size_t> file_indexes_{
        std::vector<std::size_t>(zedio::config::FIXED_FILES_NUM)};
};

} // namespace zedio::async::detail
