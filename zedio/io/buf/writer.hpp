#pragma once

#include "zedio/io/buf/stream.hpp"
#include "zedio/io/io.hpp"

namespace zedio::io {

template <class IO>
    requires requires(IO io, std::span<const char> buf) {
        { io.write(buf) };
    }
class BufWriter {
public:
    BufWriter(IO &&io, std::size_t size = detail::Config::DEFAULT_BUF_SIZE)
        : io_{std::move(io)}
        , stream_{size} {}

    // Delete Copy
    BufWriter(const BufWriter &) = delete;
    auto operator=(const BufWriter &) -> BufWriter & = delete;

public:
    [[nodiscard]]
    auto buffer() {
        return stream_.r_splice();
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
            splice = splice.subspan(ret.value(), splice.size() - ret.value());
        }
        stream_.reset_pos();
        co_return Result<void>{};
    }

    [[REMEMBER_CO_AWAIT]]
    auto write(std::span<const char> buf) {
        return real_write(buf, false);
    }

    [[REMEMBER_CO_AWAIT]]
    auto write_all(std::span<const char> buf) {
        return real_write(buf, true);
    }

    [[nodiscard]]
    auto take_writer() -> IO {
        return std::move(io_);
    }

    [[nodiscard]]
    auto writer() -> IO & {
        return io_;
    }

private:
    auto real_write(std::span<const char> buf, bool is_write_all)
        -> zedio::async::Task<Result<std::size_t>> {
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
        } while (is_write_all && stream_.w_remaining() < buf.size_bytes());

        // assert(stream_.w_remaining() >= buf.size_bytes());
        result += stream_.read_from(buf);
        co_return result;
    }

private:
    IO                io_;
    detail::BufStream stream_;
};

} // namespace zedio::io
