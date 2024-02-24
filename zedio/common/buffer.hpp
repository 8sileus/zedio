#pragma once

#include "zedio/common/config.hpp"
#include "zedio/common/error.hpp"
// C++
#include <vector>

namespace zedio {

template <class IO>
class BaseBuf {
public:
    BaseBuf(IO &&io, std::size_t buf_size = config::DEFAULT_BUF_SIZE)
        : io_{std::move(io)} buf_(buf_size) {}

protected:
    auto read_until() {
        // TODO.
    }

    template <typename T>
    auto read_buf(std::span<T> buf) -> async::Task<Result<std::size_t>> {
        if (buf.bytes_size() >= capacity()) {
            co_return co_await io_.read(buf);
        }

        Result<std::size_t> ret{};
        if (readable_bytes() < buf.bytes_size()) {
            if (writable_bytes() + readable_bytes() < buf.bytes_size()) {
                refresh();
            }
            ret = co_await read(begin_write(), writable_bytes());
        }
        if (!ret) {
            co_return ret;
        }
        w_pos_ += ret.value();
        ret = std::memcpy(buf.data(), begin_read(), buf.bytes_size());
        // assert(buf.bytes_size() == ret.value());
        r_pos_ += ret.value();
        co_return ret;
    }

private:
    void fresh() noexcept {
        auto len = readable_bytes();
        std::memcpy(buf_.begin(), buf_.begin() + r_pos_, len);
        r_pos_ = 0;
        w_pos_ = len;
    }

    auto writable_bytes() const noexcept -> std::size_t {
        return buf_.size() - w_pos_;
    }

    auto readable_bytes() const noexcept -> std::size_t {
        return w_pos_ - r_pos_;
    }

    auto read_begin() {
        return buf_.begin() + r_pos_;
    }

    auto write_begin() {
        return buf_.begin() + w_pos_;
    }

    auto capacity() const noexcept -> std::size_t {
        return buf_.szie();
    }

private:
    IO               io_;
    std::vector<int> buf_;
    std::size_t      r_pos_{};
    std::size_t      w_pos_{};
};

} // namespace zedio
