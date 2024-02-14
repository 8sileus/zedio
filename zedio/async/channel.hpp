#pragma once

#include "zedio/async/worker.hpp"
#include "zedio/common/container/ring_buffer.hpp"
#include "zedio/common/macros.hpp"
// C++
#include <concepts>
#include <coroutine>
#include <mutex>
#include <queue>
#include <vector>

namespace zedio::async {

template <typename T>
class Channel {
private:
    struct Data {
        Channel                &channel_;
        std::optional<T>        value_{std::nullopt};
        std::coroutine_handle<> handle_{nullptr};
        bool                    need_wake_up_other{false};
    };

    class ReaderAwaiter {
    public:
        ReaderAwaiter(Channel &channel)
            : data_{.channel_{channel}} {}

        constexpr auto await_ready() const noexcept -> bool {
            return false;
        }

        auto await_suspend(std::coroutine_handle<> handle) -> std::coroutine_handle<> {
            data_.handle_ = handle;
            std::lock_guard lock(data_.channel_.mutex_);

            // Check channel
            if (data_.channel_.is_closed_) {
                return handle;
            }

            // Check waiting writers
            if (!data_.channel_.waiting_writers_.empty()) {
                auto writer = data_.channel_.waiting_writers_.front();
                std::swap(data_.value_, writer->value_);
                detail::t_worker->schedule_task(std::move(writer->handle_));
                data_.channel_.waiting_writers_.pop();
                return handle;
            }

            // Check buffer
            data_.value_ = data_.channel_.buffer_.pop();
            if (data_.value_) {
                data_.need_wake_up_other = true;
                return handle;
            }

            data_.channel_.waiting_readers_.push(&data_);
            return std::noop_coroutine();
        }

        auto await_resume() -> std::optional<T> {
            if (!data_.need_wake_up_other) {
                return data_.value_;
            }

            std::lock_guard lock(data_.channel_.mutex_);

            assert(!data_.channel_.is_closed_);
            assert(data_.value_.has_value());

            if (!data_.channel_.waiting_writers_.empty() && !data_.channel_.buffer_.is_fill()) {
                auto writer = data_.channel_.waiting_writers_.front();

                assert(writer->handle_ != nullptr);

                data_.channel_.buffer_.push(writer->value_.value());
                writer->value_ = std::nullopt;
                detail::t_worker->schedule_task(std::move(writer->handle_));
                data_.channel_.waiting_writers_.pop();
            }
            return data_.value_;
        }

    private:
        Data data_;
    };

    class WriterAwaiter {
    public:
        template <typename U>
        WriterAwaiter(Channel &channel, U &&value)
            : data_{.channel_{channel}, .value_{value}} {}

        constexpr auto await_ready() const noexcept -> bool {
            return false;
        }

        auto await_suspend(std::coroutine_handle<> handle) -> std::coroutine_handle<> {
            data_.handle_ = handle;
            std::lock_guard lock(data_.channel_.mutex_);

            // Check Channel
            if (data_.channel_.is_closed_) {
                return handle;
            }

            //Check waiting readers
            if (!data_.channel_.waiting_readers_.empty()) {
                auto reader = data_.channel_.waiting_readers_.front();
                std::swap(reader->value_, data_.value_);
                detail::t_worker->schedule_task(std::move(reader->handle_));
                data_.channel_.waiting_readers_.pop();
                return handle;
            }

            // Check buffer
            if (data_.channel_.buffer_.push(std::move(data_.value_.value()))) {
                data_.value_ = std::nullopt;
                data_.need_wake_up_other = true;
                return handle;
            }
            data_.channel_.waiting_writers_.push(&data_);
            return std::noop_coroutine();
        }

        auto await_resume() -> bool {
            if (!data_.need_wake_up_other) {
                return !data_.value_.has_value();
            }

            std::lock_guard lock(data_.channel_.mutex_);

            assert(!data_.channel_.is_closed_);
            assert(!data_.value_.has_value());

            if (!data_.channel_.waiting_readers_.empty() && !data_.channel_.buffer_.is_empty()) {
                auto reader = data_.channel_.waiting_readers_.front();

                assert(reader->handle_ != nullptr);

                reader->value_ = data_.channel_.buffer_.pop();

                detail::t_worker->schedule_task(std::move(reader->handle_));
                data_.channel_.waiting_readers_.pop();
            }
            return true;
        }

    private:
        Data data_;
    };

public:
    explicit Channel(std::size_t capacity = 0)
        : buffer_{capacity} {}

    ~Channel() {
        this->close();
    }

    template <typename U>
        requires std::is_constructible_v<T, U> || std::is_convertible_v<U, T>
    [[REMEMBER_CO_AWAIT]]
    auto send(U &&value) -> WriterAwaiter {
        return WriterAwaiter{*this, std::forward<U>(value)};
    }

    [[REMEMBER_CO_AWAIT]]
    auto recv() -> ReaderAwaiter {
        return ReaderAwaiter{*this};
    }

    [[nodiscard]]
    auto capacity() const noexcept -> std::size_t {
        return buffer_.capacity();
    }

    [[nodiscard]]
    auto size() const noexcept -> std::size_t {
        std::lock_guard lock(mutex_);
        return buffer_.size();
    }

    void close() {
        if (is_closed_) {
            return;
        }
        std::lock_guard lock(mutex_);
        while (!waiting_readers_.empty()) {
            detail::t_worker->schedule_task(std::move(waiting_readers_.front()->handle_));
            waiting_readers_.pop();
        }
        while (!waiting_writers_.empty()) {
            detail::t_worker->schedule_task(std::move(waiting_writers_.front()->handle_));
            waiting_writers_.pop();
        }
        is_closed_ = true;
    }

private:
    HeapRingBuffer<T>  buffer_;
    std::queue<Data *> waiting_readers_{};
    std::queue<Data *> waiting_writers_{};
    std::mutex         mutex_{};
    bool               is_closed_{false};
};

} // namespace zedio::async
