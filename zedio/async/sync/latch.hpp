#include "zedio/async/coroutine/task.hpp"
#include "zedio/runtime/worker.hpp"

namespace zedio::async {

class Latch {
    class Awaiter {
        friend class Latch;

    public:
        explicit Awaiter(Latch *latch)
            : latch_{latch} {}

        auto await_ready() const noexcept -> bool {
            return !latch_->try_wait();
        }

        auto await_suspend(std::coroutine_handle<> handle) noexcept -> bool {
            handle_ = handle;
            next_ = latch_->head_.load(std::memory_order::acquire);
            while (!latch_->head_.compare_exchange_weak(next_,
                                                        this,
                                                        std::memory_order::acq_rel,
                                                        std::memory_order::release)) {
            }
            return latch_->try_wait();
        }

        constexpr void await_resume() const noexcept {}

    private:
        Latch                  *latch_;
        std::coroutine_handle<> handle_;
        Awaiter                *next_;
    };

public:
    Latch(std::ptrdiff_t expected)
        : expected_{expected} {}

    void count_down(std::ptrdiff_t update = 1) {
        auto odd = expected_.fetch_sub(update, std::memory_order::release);
        if (odd == update) {
            // notify all
            notify_all();
        }
    }

    [[REMEMBER_CO_AWAIT]]
    auto wait() {
        return Awaiter{this};
    }

    auto try_wait() -> bool {
        return expected_.load(std::memory_order::relaxed) != 0;
    }

    [[REMEMBER_CO_AWAIT]]
    auto arrive_and_wait() {
        count_down();
        return wait();
    }

private:
    void notify_all() {
        auto head = head_.load(std::memory_order::relaxed);
        // while (!head_.compare_exchange_weak(head,
        //                                     nullptr,
        //                                     std::memory_order::acq_rel,
        //                                     std::memory_order::release)) {
        // }
        while (head != nullptr) {
            runtime::detail::t_worker->schedule_task(head->handle_);
            head = head->next_;
        }
    }

private:
    std::atomic<std::ptrdiff_t> expected_;
    std::atomic<Awaiter *>      head_{nullptr};
};

} // namespace zedio::async
