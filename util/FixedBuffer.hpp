#pragma once

#include "noncopyable.hpp"

#include <cassert>
#include <cstring>
#include <string>

namespace zed::util {

template <std::size_t SIZE>
class FixedBuffer : util::Noncopyable {
public:
    FixedBuffer() noexcept : m_cur(m_data) {}

    // NOTE：在外部使用availableCapacity判断剩余空间是否可以容纳;
    void write(const std::string& str) noexcept {
        assert(writableBytes() > str.size());
        std::memcpy(m_cur, str.data(), str.size());
        m_cur += str.size();
    };

    [[nodiscard]] std::size_t writableBytes() const noexcept { return m_data + sizeof(m_data) - m_cur; }

    [[nodiscard]] const char* data() const noexcept { return m_data; }

    [[nodiscard]] std::size_t size() const noexcept { return m_cur - m_data; }

    [[nodiscard]] constexpr std::size_t capacity() noexcept { return SIZE; }

    [[nodiscard]] bool empty() const noexcept { return m_cur == m_data; }

    void reset() noexcept { m_cur = m_data; }

    void bzero() noexcept { std::memset(m_data, 0, sizeof(m_data)); }

private:
    char  m_data[SIZE]{};
    char* m_cur{nullptr};
};

}  // namespace zed::util
