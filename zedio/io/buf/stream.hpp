#pragma once

// C
#include <cassert>
#include <cstring>
// C++
#include <algorithm>
#include <span>
#include <vector>

namespace zedio::io::detail {

class BufStream {
public:
    BufStream(std::size_t size)
        : buf_(size) {}

public:
    auto write_to(std::span<char> buf) -> std::size_t {
        auto len = std::min(readable_bytes(), buf.size_bytes());
        std::copy(begin_read(), begin_read() + len, buf.data());
        increase_rpos(buf.size_bytes());
        if (r_pos_ == w_pos_) {
            reset_pos();
        }
        return len;
    }

    auto writable_bytes() const noexcept -> std::size_t {
        return buf_.size() - w_pos_;
    }

    auto readable_bytes() const noexcept -> std::size_t {
        return w_pos_ - r_pos_;
    }

    auto remaining_bytes() {
        return buf_.size() - readable_bytes();
    }

    auto begin_write() -> char * {
        return buf_.data() + w_pos_;
    }

    auto begin_read() -> char * {
        return buf_.data() + r_pos_;
    }

    auto begin_read() const -> const char * {
        return buf_.data() + r_pos_;
    }

    void increase_rpos(std::size_t len) {
        r_pos_ += len;
    }

    void increase_wpos(std::size_t len) {
        w_pos_ += len;
    }

    auto capacity() -> std::size_t {
        return buf_.size();
    }

    void move_to_front() {
        auto len = readable_bytes();
        ::memcpy(buf_.data(), begin_read(), len);
        r_pos_ = 0;
        w_pos_ = len;
    }

    auto empty() -> bool {
        return r_pos_ == w_pos_;
    }

    auto find_flag_and_return_splice(char flag) const -> std::span<const char> {
        if (auto end
            = std::find(buf_.begin() + r_pos_, buf_.begin() + r_pos_ + readable_bytes(), flag);
            end == buf_.end()) {
            return {};
        } else {
            return {buf_.begin() + r_pos_, end - (buf_.begin() + r_pos_) + 1};
        }
    }

    auto write_splice() -> std::span<char> {
        return {begin_write(), writable_bytes()};
    }

    auto read_splice() -> std::span<char> {
        return {begin_read(), readable_bytes()};
    }

    auto read_splice() const -> std::span<const char> {
        return {begin_read(), readable_bytes()};
    }

    void reset_pos() {
        r_pos_ = w_pos_ = 0;
    }

private:
    std::vector<char> buf_;
    std::size_t       r_pos_{0};
    std::size_t       w_pos_{0};
};

} // namespace zedio::io::detail
