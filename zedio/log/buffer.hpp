#pragma once

// C
#include <cassert>
#include <cstring>
// C++
#include <array>
#include <string>

namespace zedio::log::detail {

template <std::size_t SIZE>
class LogBuffer {
public:
    LogBuffer() noexcept
        : cur_{data_.begin()} {}

    // NOTE：在外部使用availableCapacity判断剩余空间是否可以容纳;
    void write(const std::string &str) noexcept {
        assert(writable_bytes() > str.size());
        std::copy(str.begin(), str.end(), cur_);
        cur_ += str.size();
    };

    [[nodiscard]]
    constexpr auto capacity() noexcept -> std::size_t {
        return SIZE;
    }

    [[nodiscard]]
    auto size() noexcept -> std::size_t {
        return std::distance(data_.begin(), cur_);
    }

    [[nodiscard]]
    auto writable_bytes() noexcept -> std::size_t {
        return capacity() - size();
    }

    [[nodiscard]]
    auto data() const noexcept -> const char * {
        return data_.data();
    }

    [[nodiscard]]
    auto empty() const noexcept -> bool {
        return cur_ == data_.begin();
    }

    void reset() noexcept {
        cur_ = data_.begin();
    }

private:
    std::array<char, SIZE>           data_{};
    std::array<char, SIZE>::iterator cur_{};
};

} // namespace zedio::log::detail
