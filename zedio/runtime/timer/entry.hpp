#pragma once

// C
#include <cassert>
// C++
#include <chrono>
#include <concepts>
#include <coroutine>
#include <memory>

namespace zedio::runtime::detail {

class Entry {
public:
    Entry(std::chrono::steady_clock::time_point expiration_time, std::coroutine_handle<> handle)
        : expiration_time_{expiration_time}
        , handle_{handle} {}

    Entry(std::chrono::steady_clock::time_point expiration_time, io::detail::IOData *data)
        : expiration_time_{expiration_time}
        , data_{data} {}

public:
    template <typename LocalQueue, typename GlobalQueue>
    void execute(LocalQueue &local_queue, GlobalQueue &global_queue) {
        if (handle_ != nullptr) {
            assert(data_ == nullptr);
            local_queue.push_back_or_overflow(handle_, global_queue);
        } else {
#ifdef __linux__
            assert(handle_ == nullptr);
            auto sqe = runtime::detail::t_ring->get_sqe();
            io_uring_prep_cancel(sqe, data_, 0);
            io_uring_sqe_set_data(sqe, nullptr);
            data_->entry = nullptr;
#elif _WIN32
            ::CancelIoEx(data_->handle, &data_->overlapped);
#endif
        }
    }

public:
    template <class T>
        requires std::constructible_from<Entry, std::chrono::steady_clock::time_point, T>
    [[nodiscard]]
    static auto make(std::chrono::steady_clock::time_point expiration_time,
                     T handle) -> std::pair<std::unique_ptr<Entry>, Entry *> {
        auto entry = std::make_unique<Entry>(expiration_time, handle);
        return std::make_pair(std::move(entry), entry.get());
    }

public:
    std::chrono::steady_clock::time_point expiration_time_;
    std::coroutine_handle<>               handle_{nullptr};
    io::detail::IOData                   *data_{nullptr};
    std::unique_ptr<Entry>                next_{nullptr};
};

} // namespace zedio::runtime::detail
