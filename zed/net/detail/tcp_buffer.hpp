#pragma once

#include "net/detail/config.hpp"

// C
#include <cassert>
#include <cstring>
// C++
#include <string>
#include <string_view>
#include <vector>

namespace zed::net::detail {

class TcpBuffer {
public:
    explicit TcpBuffer(std::size_t initial_size = detail::TCP_BUFFER_INITIAL_SIZE)
        : buffer_(initial_size) {}

    [[nodiscard]]
    auto readable_bytes() const -> std::size_t {
        assert(write_index_ > read_index_);
        return write_index_ - read_index_;
    }

    [[nodiscard]]
    auto writeable_bytes() const -> std::size_t {
        return buffer_.size() - write_index_;
    }

    [[nodiscard]]
    auto prependable_bytes() const -> std::size_t {
        return read_index_;
    }

    [[nodiscard]]
    auto size() const -> std::size_t {
        return buffer_.size();
    }

    [[nodiscard]]
    auto begin_write() -> char * {
        return buffer_.data() + write_index_;
    }

    [[nodiscard]]
    auto begin_read() const -> const char * {
        return buffer_.data() + read_index_;
    }

    void has_written(std::size_t len) {
        assert(len <= writeable_bytes());
        write_index_ += len;
    }

    void append(char *data, std::size_t len) {
        if (writeable_bytes() < len) {
            expand(len);
        }
        ::memcpy(begin_write(), data, len);
        has_written(len);
    }

    void retrieve(std::size_t len) {
        if (len < readable_bytes()) {
            read_index_ += len;
        } else {
            retrieve_all();
        }
    }

    void retrieve_all() { read_index_ = write_index_ = 0; }

    auto as_string_view() const -> std::string_view {
        return std::string_view(buffer_.data() + read_index_, readable_bytes());
    }

    auto as_string() const -> std::string { return std::string(as_string_view()); }

private:
    void expand(std::size_t len) {
        if (writeable_bytes() < prependable_bytes()) {
            buffer_.resize(write_index_ + len);
        } else {
            auto n = writeable_bytes();
            ::memcpy(buffer_.data(), buffer_.data() + read_index_, n);
            read_index_ = 0;
            write_index_ = n;
        }
    }

private:
    std::size_t       read_index_{};
    std::size_t       write_index_{};
    std::vector<char> buffer_;
};

} // namespace zed::net::detail
