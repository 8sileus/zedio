#pragma once

#include "async/detail/dispatcher.hpp"
#include "async/detail/processor.hpp"
#include "util/parse_config.hpp"
// C++
#include <future>

namespace zed::async {

class Scheduler : util::Noncopyable {
public:
    Scheduler() {
        auto parse_config = util::ParseConfig(util::ZED_CONFIG_FILE);

        auto worker_num = parse_config.get<unsigned int>("scheduelr.worker_num",
                                                         std::thread::hardware_concurrency());

        auto entries = parse_config.get<unsigned>("processor.entries", 1024 * worker_num);

        auto flags
            = parse_config.get<bool>("processor.open_sqpoll", true) ? IORING_SETUP_SQPOLL : 0;

        if (auto res = io_uring_queue_init(entries, &uring_, flags); res != 0) [[unlikely]] {
            throw std::runtime_error(std::format(
                "call io_uring_queue_init failed, errno: {}, message: {}", -res, strerror(-res)));
        }

        for (std::size_t i = 0; i < worker_num; ++i) {
            std::promise<void> p;
            threads_.emplace_back(&Scheduler::create_worker, this, std::ref(p));
            p.get_future().get();
        }
    }

    ~Scheduler() {
        dispatcher_.stop();
        for (auto &thread : threads_) {
            thread.join();
        }
        io_uring_queue_exit(&uring_);
    }

    void run() {
        if (running_) [[unlikely]] {
            log::zed.debug("scheduler has been running");
            return;
        }
        running_ = true;
        io_uring_cqe *cqe;
        while (running_) {
            io_uring_wait_cqe(&uring_, &cqe);
            auto io_awaiter
                = reinterpret_cast<detail::BaseIOAwaiter *>(io_uring_cqe_get_data64(cqe));
            io_awaiter->res_ = cqe->res;
            auto task = [io_awaiter]() { io_awaiter->handle_.resume(); };
            dispatcher_.dispatch(task, io_awaiter->tid_);
            io_uring_cqe_seen(&uring_, cqe);
        }
    }

    void stop() { running_ = false; }

    void push(Task<void> task) {
        auto handle = task.take();
        auto f = [handle]() { handle.resume(); };
        push(f);
    }

    void push(std::function<void()> task) { dispatcher_.dispatch(std::move(task), 0); }

private:
    void create_worker(std::promise<void> &p) {
        detail::Processor processor(&uring_);
        dispatcher_.add_processor(&processor);
        dispatcher_.map_processor(processor.tid(), &processor);
        p.set_value();
        processor.run();
    }

private:
    io_uring                 uring_{};
    std::vector<std::thread> threads_{};
    detail::Dispatcher       dispatcher_{};
    std::atomic<bool>        running_{false}; // TODO consider delete this variable
};

} // namespace zed::async
