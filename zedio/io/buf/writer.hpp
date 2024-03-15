#pragma once

#include "zedio/io/buf/buffer.hpp"

namespace zedio::io::detail {

template <typename B>
class Writer {
public:
    [[nodiscard]]
    auto buffer() noexcept {
        return static_cast<B *>(this)->w_stream_.r_splice();
    }

    [[nodiscard]]
    auto capacity() const noexcept -> std::size_t {
        return static_cast<const B *>(this)->w_stream_.capacity();
    }

    [[REMEMBER_CO_AWAIT]]
    auto flush() -> zedio::async::Task<Result<void>> {
        auto                splice = buffer();
        Result<std::size_t> ret;
        while (!splice.empty()) {
            ret = co_await static_cast<B *>(this)->io_.write(splice);
            if (!ret) [[unlikely]] {
                co_return std::unexpected{ret.error()};
            }
            if (ret.value() == 0) {
                co_return std::unexpected{make_zedio_error(Error::WriteZero)};
            }
            static_cast<B *>(this)->w_stream_.r_increase(ret.value());
            splice = splice.subspan(ret.value(), splice.size_bytes() - ret.value());
        }
        static_cast<B *>(this)->w_stream_.reset_pos();
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

private:
    template <bool write_once>
    auto real_write(std::span<const char> buf) -> zedio::async::Task<Result<std::size_t>> {
        if (static_cast<B *>(this)->w_stream_.w_remaining() >= buf.size_bytes()) {
            co_return static_cast<B *>(this)->w_stream_.read_from(buf);
        }
        std::array<struct iovec, 2> iovecs{};
        std::size_t                 result = 0;
        Result<std::size_t>         ret;
        do {
            iovecs[0].iov_base
                = const_cast<char *>(static_cast<B *>(this)->w_stream_.r_splice().data());
            iovecs[0].iov_len = static_cast<B *>(this)->w_stream_.r_splice().size_bytes();
            iovecs[1].iov_base = const_cast<char *>(buf.data());
            iovecs[1].iov_len = buf.size_bytes();

            ret = co_await io::writev(static_cast<B *>(this)->io_.fd(),
                                      iovecs.data(),
                                      iovecs.size(),
                                      -1);
            if (!ret) {
                co_return std::unexpected{ret.error()};
            }
            if (ret.value() == 0) {
                co_return std::unexpected{make_zedio_error(Error::WriteZero)};
            }
            auto len = std::min(ret.value(), static_cast<B *>(this)->w_stream_.r_remaining());
            static_cast<B *>(this)->w_stream_.r_increase(len);
            static_cast<B *>(this)->w_stream_.reset_data();
            ret.value() -= len;
            result += ret.value();
            buf = buf.subspan(ret.value(), buf.size_bytes() - ret.value());
            if constexpr (write_once) {
                break;
            }
        } while (static_cast<B *>(this)->w_stream_.w_remaining() < buf.size_bytes());

        // assert(w_stream_.w_remaining() >= buf.size_bytes());
        result += static_cast<B *>(this)->w_stream_.read_from(buf);
        co_return result;
    }
};

} // namespace zedio::io::detail
