#pragma once

#include "zedio/io/buf/buffer.hpp"

namespace zedio::io::detail {

template <class IO>
    requires requires(IO io, std::span<char> buf) {
        { io.read(buf) };
    }
class Reader {
public:
    Reader(IO &io, StreamBuffer &stream)
        : io_{io}
        , stream_{stream} {}

    // Delete Copy
    Reader(const Reader &) = delete;
    auto operator=(const Reader &) -> Reader & = delete;

public:
    [[nodiscard]]
    auto buffer() {
        return stream_.r_splice();
    }

    [[nodiscard]]
    auto capacity() const noexcept -> std::size_t {
        return stream_.capacity();
    }

    void consume(std::size_t n) {
        n = std::min(n, stream_.r_remaining());
        stream_.r_increase(n);
    }

    [[REMEMBER_CO_AWAIT]]
    auto fill_buf() -> zedio::async::Task<Result<void>> {
        auto ret = co_await io::read(io_.fd(),
                                     stream_.w_splice().data(),
                                     stream_.w_splice().size_bytes(),
                                     static_cast<uint64_t>(-1));
        if (!ret) [[unlikely]] {
            co_return std::unexpected{ret.error()};
        }
        co_return Result<void>{};
    }

    [[REMEMBER_CO_AWAIT]]
    auto read(std::span<char> buf) {
        return real_read<false>(buf, [this]() -> bool { return !this->stream_.empty(); });
    }

    [[REMEMBER_CO_AWAIT]]
    auto read_exact(std::span<char> buf) {
        return real_read<true>(buf, [this, exact_bytes = buf.size_bytes()]() -> bool {
            return this->stream_.r_remaining() >= exact_bytes;
        });
    }

    template <typename T>
        requires requires(std::string_view s, T s_or_c) {
            { s.find(s_or_c) };
        }
    [[REMEMBER_CO_AWAIT]]
    auto read_until(std::string &buf, T end_flag) -> zedio::async::Task<Result<std::size_t>> {
        Result<std::size_t> ret;
        while (true) {
            if (auto splice = stream_.find_flag_and_return_splice(end_flag); !splice.empty()) {
                buf.append(splice.begin(), splice.end());
                stream_.r_increase(splice.size_bytes());
                break;
            } else {
                buf.append(stream_.r_splice().begin(), stream_.r_splice().end());
                stream_.reset_pos();
            }

            ret = co_await io_.read(stream_.w_splice());
            if (!ret) [[unlikely]] {
                co_return std::unexpected{ret.error()};
            }
            if (ret.value() == 0) {
                break;
            }
            stream_.w_increase(ret.value());
        }
        co_return buf.size();
    }

    [[REMEMBER_CO_AWAIT]]
    auto read_line(std::string &buf) {
        return read_until(buf, '\n');
    }

    [[nodiscard]]
    auto reader() -> IO & {
        return io_;
    }

private:
    template <bool eof_is_error, class Pred>
        requires std::is_invocable_v<Pred>
    auto real_read(std::span<char> buf, Pred pred) -> zedio::async::Task<Result<std::size_t>> {
        if (stream_.capacity() < buf.size_bytes()) {
            auto len = stream_.write_to(buf);
            buf = buf.subspan(len, buf.size_bytes() - len);
            co_return co_await io_.read(buf);
        }

        if (pred()) {
            co_return stream_.write_to(buf);
        }

        if (stream_.w_remaining() < buf.size_bytes()) {
            stream_.reset_data();
        }

        assert(stream_.w_remaining() >= buf.size_bytes());

        Result<std::size_t> ret;
        while (true) {
            ret = co_await io_.read(stream_.w_splice());
            if (!ret) [[unlikely]] {
                co_return std::unexpected{ret.error()};
            }
            stream_.w_increase(ret.value());

            if constexpr (eof_is_error) {
                if (ret.value() == 0) {
                    co_return std::unexpected{make_zedio_error(Error::UnexpectedEOF)};
                }
                if (pred()) {
                    co_return stream_.write_to(buf);
                }
            } else {
                if (pred() || ret.value() == 0) {
                    co_return stream_.write_to(buf);
                }
            }
        }
    }

private:
    IO           &io_;
    StreamBuffer &stream_;
};

} // namespace zedio::io::detail
