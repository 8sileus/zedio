#pragma once

#include "util/noncopyable.hpp"

#include <cassert>
#include <cstring>
#include <string>

namespace zed::log::detail {

template <std::size_t SIZE>
class LogBuffer : util::Noncopyable {
public:
    LogBuffer() noexcept : m_cur{m_data} {}

    // NOTE：在外部使用availableCapacity判断剩余空间是否可以容纳;
    void write(const std::string& str) noexcept {
        assert(writableBytes() > str.size());
        std::memcpy(m_cur, str.data(), str.size());
        m_cur += str.size();
    };

    [[nodiscard]] constexpr auto capacity() const noexcept -> std::size_t {
        return SIZE;
    }

    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return m_cur - m_data;
    }

    [[nodiscard]] auto writeableSize() const noexcept -> std::size_t {
        return capacity() - size();
    }

    [[nodiscard]] auto data() const noexcept -> const char* { return m_data; }

    [[nodiscard]] auto empty() const noexcept -> bool {
        return m_cur == m_data;
    }

    void reset() noexcept { m_cur = m_data; }

private:
    char  m_data[SIZE]{};
    char* m_cur{nullptr};
};

}  // namespace zed::log::detail
