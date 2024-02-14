#pragma once

// C++
#include <algorithm>
#include <array>
#include <vector>

namespace zedio {

template <typename T, std::size_t SIZE>
class FixedRingBuffer {
public:
#define RSIZE (SIZE + 1)

    [[nodiscard]]
    auto empty() const -> bool {
        return start_ == end_;
    }

    [[nodiscard]]
    auto fill() const -> bool {
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
    auto capacity() const -> std::size_t {
        return SIZE;
    }

public:
    std::size_t          start_{0};
    std::size_t          end_{0};
    std::array<T, RSIZE> data_{};
#undef RSIZE
};

template <typename T>
class FlexRingBuffer {
public:
    explicit FlexRingBuffer(std::size_t size)
        : data_(size + 1) {}

    [[nodiscard]]
    auto empty() const -> bool {
        return start_ == end_;
    }

    [[nodiscard]]
    auto fill() const -> bool {
        return (end_ + 1) % data_.size() == start_;
    }

    void push(const T &val) {
        data_[end_++] = val;
        end_ %= data_.size();
    }

    [[nodiscard]]
    auto take() -> T {
        auto res = std::move(data_[start_++]);
        start_ %= data_.size();
        return res;
    }

    [[nodiscard]]
    auto size() const -> std::size_t {
        return (end_ - start_ + data_.size()) % data_.size();
    }

    [[nodiscard]]
    auto capacity() const -> std::size_t {
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

} // namespace zedio
