#pragma once

// C
#include <cassert>
#include <cstring>
// C++
#include <algorithm>
#include <span>
#include <string_view>
#include <vector>

namespace zedio::io::detail {

class BufStream {
public:
    BufStream(std::size_t size)
        : buf_(size) {}

public:
    void reset_data() {
        auto len = r_remaining();
        std::copy(r_begin(), r_end(), buf_.begin());
        r_pos_ = 0;
        w_pos_ = len;
    }

    void reset_pos() noexcept {
        r_pos_ = w_pos_ = 0;
    }

    void r_increase(std::size_t n) {
        r_pos_ += n;
        assert(r_pos_ <= w_pos_);
    }

    [[nodiscard]]
    auto r_remaining() const noexcept -> std::size_t {
        return w_pos_ - r_pos_;
    }

    [[nodiscard]]
    auto r_begin() const -> std::vector<char>::const_iterator {
        return buf_.begin() + r_pos_;
    }

    [[nodiscard]]
    auto r_end() const -> const std::vector<char>::const_iterator {
        return r_begin() + r_remaining();
    }

    [[nodiscard]]
    auto r_splice() const -> std::span<const char> {
        return {r_begin(), r_end()};
    }

    void w_increase(std::size_t n) noexcept {
        w_pos_ += n;
        assert(r_pos_ <= w_pos_);
    }

    [[nodiscard]]
    auto w_remaining() const noexcept -> std::size_t {
        return buf_.size() - w_pos_;
    }

    [[nodiscard]]
    auto w_begin() -> std::vector<char>::iterator {
        return buf_.begin() + w_pos_;
    }

    [[nodiscard]]
    auto w_end() -> std::vector<char>::iterator {
        return w_begin() + w_remaining();
    }

    [[nodiscard]]
    auto w_splice() -> std::span<char> {
        return {w_begin(), w_end()};
    }

    [[nodiscard]]
    auto empty() const noexcept -> bool {
        return r_pos_ == w_pos_;
    }

    [[nodiscard]]
    auto capacity() const noexcept -> std::size_t {
        return buf_.size();
    }

    [[nodiscard]]
    auto write_to(std::span<char> dst_buf) -> std::size_t {
        auto len = std::min(r_remaining(), dst_buf.size_bytes());
        std::copy_n(r_begin(), len, dst_buf.begin());
        r_increase(len);
        if (r_pos_ == w_pos_) {
            reset_pos();
        }
        return len;
    }

    [[nodiscard]]
    auto read_from(std::span<const char> src_buf) -> std::size_t {
        auto len = std::min(w_remaining(), src_buf.size_bytes());
        std::copy_n(src_buf.begin(), len, w_begin());
        w_increase(len);
        return len;
    }

    [[nodiscard]]
    auto find_flag_and_return_splice(std::string_view end_str) -> std::span<const char> {
        auto pos = std::string_view{r_splice()}.find(end_str);
        if (pos == std::string_view::npos) {
            return {};
        } else {
            return {r_begin(), pos + end_str.size()};
        }
    }

    [[nodiscard]]
    auto find_flag_and_return_splice(char end_char) -> std::span<const char> {
        auto pos = std::string_view{r_splice()}.find(end_char);
        if (pos == std::string_view::npos) {
            return {};
        } else {
            return {r_begin(), pos + 1};
        }
    }

private:
    std::vector<char> buf_;
    std::size_t       r_pos_{0};
    std::size_t       w_pos_{0};
};

} // namespace zedio::io::detail
