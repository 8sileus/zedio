#pragma once

#include "async/detail/processor.hpp"

// C
#include <cassert>
// C++
#include <unordered_map>

namespace zed::async::detail {

class Dispatcher {
public:
    void dispatch(std::function<void()> &&task, pid_t tid) {
        std::lock_guard lock(mutex_);
        if (tid) {
            assert(processor_map_.count(tid));
            processor_map_[tid]->push(std::move(task));
        } else {
            processor_vec_[index_++]->push(std::move(task));
            index_ %= processor_vec_.size();
        }
    }

    void stop() {
        for (auto processor : processor_vec_) {
            auto stop_task = [processor]() { processor->stop(); };
            processor->push(stop_task);
        }
    }

    void add_processor(Processor *processor) { processor_vec_.emplace_back(processor); }

    void map_processor(pid_t tid, Processor *processor) { processor_map_.emplace(tid, processor); }

private:
    util::SpinMutex                        mutex_{};
    std::vector<Processor *>               processor_vec_{};
    std::unordered_map<pid_t, Processor *> processor_map_{};
    std::size_t                            index_{0};
};

} // namespace zed::async::detail
