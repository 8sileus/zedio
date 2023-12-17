#pragma once

#include "async/detail/processor.hpp"
#include "common/util/noncopyable.hpp"
#include "common/util/thread.hpp"
// C++
#include <barrier>
#include <unordered_map>

namespace zed::async::detail {

class Dispatcher;


class Dispatcher : util::Noncopyable {
public:
    Dispatcher(std::size_t worker_num = std::thread::hardware_concurrency())
        : worker_num_{worker_num} {
        this->processors_.reserve(worker_num);
        this->threads_.reserve(worker_num);
        std::barrier b(worker_num + 1);
        for (std::size_t i{0}; i < worker_num; i += 1) {
            threads_.emplace_back([&]() {
                Processor processor;
                this->processors_.emplace_back(&processor);
                this->map_.emplace(processor.get_tid(), &processor);
                b.arrive_and_drop();
                processor.start();
            });
        }
        b.arrive_and_wait();
    }

    ~Dispatcher() {
        for (auto &processor : this->processors_) {
            processor->stop();
        }

        for (auto &thread : this->threads_) {
            thread.join();
        }
    }

    void dispatch(Task<void> &&coro) {
        this->processors_[index_]->post(std::move(coro));
        this->index_ = (this->index_ + 1) % this->worker_num_;
    }

private:
    std::vector<Processor *>               processors_{};
    std::vector<std::thread>               threads_{};
    std::unordered_map<pid_t, Processor *> map_{};
    std::size_t                            index_{0};
    std::size_t                            worker_num_;
};

} // namespace zed::async::detail
