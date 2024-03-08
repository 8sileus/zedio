#pragma once

// C++
#include <algorithm>
#include <array>
#include <concepts>
#include <optional>
#include <vector>

namespace zedio::util {

template <typename T, std::size_t SIZE>
class StackRingBuffer {
public:
#define RSIZE (SIZE + 1)

    [[nodiscard]]
    auto is_empty() const -> bool {
        return start_ == end_;
    }

    [[nodiscard]]
    auto is_fill() const -> bool {
        return (end_ + 1) % RSIZE == start_;
    }

    void push(const T &val) {
        data_[end_++] = val;
        end_ %= RSIZE;
    }

    [[nodiscard]]
    auto take() -> T {
        auto res = std::move(data_[start_++]);
        start_ %= RSIZE;
        return res;
    }

    [[nodiscard]]
    auto size() const -> std::size_t {
        return (end_ - start_ + RSIZE) % RSIZE;
    }

    [[nodiscard]]
    constexpr auto capacity() const noexcept -> std::size_t {
        return SIZE;
    }

private:
    std::size_t          start_{0};
    std::size_t          end_{0};
    std::array<T, RSIZE> data_{};
#undef RSIZE
};

template <typename T>
class HeapRingBuffer {
public:
    explicit HeapRingBuffer(std::size_t size)
        : data_(size + 1) {}

    [[nodiscard]]
    auto is_empty() const -> bool {
        return start_ == end_;
    }

    [[nodiscard]]
    auto is_fill() const -> bool {
        return (end_ + 1) % data_.size() == start_;
    }

    template <typename U>
        requires std::is_nothrow_constructible_v<T, U>
    void safety_push(U &&val) {
        assert(!is_fill());
        data_[end_++] = std::forward<U>(val);
        end_ %= data_.size();
    }

    [[nodiscard]]
    auto safety_pop() -> T {
        assert(!is_empty());
        auto ret = std::move(data_[start_++]);
        start_ %= data_.size();
        return ret;
    }

    template <typename U>
        requires std::is_nothrow_constructible_v<T, U>
    auto push(U &&val) -> bool {
        if (is_fill()) {
            return false;
        }
        data_[end_++] = std::forward<U>(val);
        end_ %= data_.size();
        return true;
    }

    [[nodiscard]]
    auto pop() -> std::optional<T> {
        if (is_empty()) {
            return std::nullopt;
        }
        auto res = std::move(data_[start_++]);
        start_ %= data_.size();
        return res;
    }

    [[nodiscard]]
    auto size() const noexcept -> std::size_t {
        return (end_ - start_ + data_.size()) % data_.size();
    }

    [[nodiscard]]
    auto capacity() const noexcept -> std::size_t {
        return data_.size() - 1;
    }

    [[nodiscard]]
    auto resize(std::size_t new_size) -> bool {
        new_size += 1;
        if (new_size == data_.size()) {
            return true;
        }
        if (new_size < size()) {
            return false;
        }
        std::vector<T> data(new_size);
        if (start_ < end_) {
            std::copy(data_.begin() + start_, data_.begin() + end_, data.begin());
            end_ = end_ - start_;
            start_ = 0;
        } else if (start_ > end_) {
            std::copy(data_.begin() + start_, data_.end(), data.begin());
            std::copy(data_.begin(), data_.begin() + end_, data.begin() + data_.size() - start_);
            end_ += data_.size() - start_;
            start_ = 0;
        }
        data_.swap(data);
        return true;
    }

private:
    std::size_t    start_{0};
    std::size_t    end_{0};
    std::vector<T> data_{};
};

} // namespace zedio::util
