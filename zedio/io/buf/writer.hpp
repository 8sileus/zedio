#pragma once

#include "zedio/io/buf/buffer.hpp"

namespace zedio::io::detail {

template <class IO>
    requires requires(IO io, std::span<const char> buf) {
        { io.write(buf) };
    }
class Writer {
public:
    Writer(IO &io, StreamBuffer &stream)
        : io_{io}
        , stream_{stream} {}

    // Delete Copy
    Writer(const Writer &) = delete;
    auto operator=(const Writer &) -> Writer & = delete;

public:
    [[nodiscard]]
    auto buffer() {
        return stream_.r_splice();
    }

    [[nodiscard]]
    auto capacity() const noexcept -> std::size_t {
        return stream_.capacity();
    }

    [[REMEMBER_CO_AWAIT]]
    auto flush() -> zedio::async::Task<Result<void>> {
        auto                splice = stream_.r_splice();
        Result<std::size_t> ret;
        while (!splice.empty()) {
            ret = co_await io_.write(splice);
            if (!ret) [[unlikely]] {
                co_return std::unexpected{ret.error()};
            }
            if (ret.value() == 0) {
                co_return std::unexpected{make_zedio_error(Error::WriteZero)};
            }
            stream_.r_increase(ret.value());
            splice = splice.subspan(ret.value(), splice.size_bytes() - ret.value());
        }
        stream_.reset_pos();
        co_return Result<void>{};
    }

    [[REMEMBER_CO_AWAIT]]
    auto write(std::span<const char> buf) {
        return real_write<true>(buf);
    }

    [[REMEMBER_CO_AWAIT]]
    auto write_all(std::span<const char> buf) {
        return real_write<false>(buf);
    }

    [[nodiscard]]
    auto writer() -> IO & {
        return io_;
    }

private:
    template <bool write_once>
    auto real_write(std::span<const char> buf) -> zedio::async::Task<Result<std::size_t>> {
        if (stream_.w_remaining() >= buf.size_bytes()) {
            co_return stream_.read_from(buf);
        }
        std::array<struct iovec, 2> iovecs{};
        std::size_t                 result = 0;
        Result<std::size_t>         ret;
        do {
            iovecs[0].iov_base = const_cast<char *>(stream_.r_splice().data());
            iovecs[0].iov_len = stream_.r_splice().size_bytes();
            iovecs[1].iov_base = const_cast<char *>(buf.data());
            iovecs[1].iov_len = buf.size_bytes();

            ret = co_await io::writev(io_.fd(), iovecs.data(), iovecs.size(), -1);
            if (!ret) {
                co_return std::unexpected{ret.error()};
            }
            if (ret.value() == 0) {
                co_return std::unexpected{make_zedio_error(Error::WriteZero)};
            }
            auto len = std::min(ret.value(), stream_.r_remaining());
            stream_.r_increase(len);
            stream_.reset_data();
            ret.value() -= len;
            result += ret.value();
            buf = buf.subspan(ret.value(), buf.size_bytes() - ret.value());
            if constexpr (write_once) {
                break;
            }
        } while (stream_.w_remaining() < buf.size_bytes());

        // assert(stream_.w_remaining() >= buf.size_bytes());
        result += stream_.read_from(buf);
        co_return result;
    }

private:
    IO           &io_;
    StreamBuffer &stream_;
};

} // namespace zedio::io::detail
