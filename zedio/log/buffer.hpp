#pragma once

// C
#include <cassert>
#include <cstring>
// C++
#include <string>

namespace zedio::log::detail {

template <std::size_t SIZE>
class LogBuffer {
public:
    LogBuffer() noexcept
        : cur_{data_} {}

    // NOTE：在外部使用availableCapacity判断剩余空间是否可以容纳;
    void write(const std::string &str) noexcept {
        assert(writeable_bytes() > str.size());
        std::memcpy(cur_, str.data(), str.size());
        cur_ += str.size();
    };

    [[nodiscard]]
    constexpr auto capacity() const noexcept -> std::size_t {
        return SIZE;
    }

    [[nodiscard]]
    auto size() const noexcept -> std::size_t {
        return cur_ - data_;
    }

    [[nodiscard]]
    auto writeable_bytes() const noexcept -> std::size_t {
        return capacity() - size();
    }

    [[nodiscard]]
    auto data() const noexcept -> const char * {
        return data_;
    }

    [[nodiscard]]
    auto empty() const noexcept -> bool {
        return cur_ == data_;
    }

    void reset() noexcept { cur_ = data_; }

private:
    char  data_[SIZE]{};
    char *cur_{nullptr};
};

} // namespace zedio::log::detail
