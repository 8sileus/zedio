#pragma once

#include "zedio/io/buf/buffer.hpp"

namespace zedio::io::detail {

template <class B>
class ImplBufRead {
public:
    [[nodiscard]]
    auto buffer() noexcept {
        return static_cast<B *>(this)->r_stream_.r_splice();
    }

    [[nodiscard]]
    auto capacity() const noexcept -> std::size_t {
        return static_cast<const B *>(this)->r_stream_.capacity();
    }

    void consume(std::size_t n) noexcept {
        n = std::min(n, static_cast<B *>(this)->r_stream_.r_remaining());
        static_cast<B *>(this)->r_stream_.r_increase(n);
    }

    [[REMEMBER_CO_AWAIT]]
    auto fill_buf() -> zedio::async::Task<Result<void>> {
        auto ret = co_await io::read(static_cast<B *>(this)->io_.fd(),
                                     static_cast<B *>(this)->r_stream_.w_splice().data(),
                                     static_cast<B *>(this)->r_stream_.w_splice().size_bytes(),
                                     static_cast<uint64_t>(-1));
        if (!ret) [[unlikely]] {
            co_return std::unexpected{ret.error()};
        }
        co_return Result<void>{};
    }

    [[REMEMBER_CO_AWAIT]]
    auto read(std::span<char> buf) {
        return real_read<false>(buf, [this]() -> bool {
            return !static_cast<B *>(this)->r_stream_.empty();
        });
    }

    [[REMEMBER_CO_AWAIT]]
    auto read_exact(std::span<char> buf) {
        return real_read<true>(buf, [this, exact_bytes = buf.size_bytes()]() -> bool {
            return static_cast<B *>(this)->r_stream_.r_remaining() >= exact_bytes;
        });
    }

    template <typename C, typename T>
        requires requires(std::string_view s, T s_or_c) {
            { s.find(s_or_c) };
        } && (std::is_same_v<std::string, C> || std::is_same_v<std::vector<char>, C>)
    [[REMEMBER_CO_AWAIT]] auto read_until(C &buf, T end_flag)
        -> zedio::async::Task<Result<std::size_t>> {
        Result<std::size_t> ret;
        while (true) {
            if (auto splice
                = static_cast<B *>(this)->r_stream_.find_flag_and_return_splice(end_flag);
                !splice.empty()) {
                if constexpr (std::is_same_v<std::string, C>) {
                    buf.append(splice.begin(), splice.end());
                } else {
                    buf.insert(buf.end(), splice.begin(), splice.end());
                }
                static_cast<B *>(this)->r_stream_.r_increase(splice.size_bytes());
                break;
            } else {
                if constexpr (std::is_same_v<std::string, C>) {
                    buf.append(static_cast<B *>(this)->r_stream_.r_splice().begin(),
                               static_cast<B *>(this)->r_stream_.r_splice().end());
                } else {
                    buf.insert(buf.end(),
                               static_cast<B *>(this)->r_stream_.r_splice().begin(),
                               static_cast<B *>(this)->r_stream_.r_splice().end());
                }
                static_cast<B *>(this)->r_stream_.reset_pos();
            }

            ret = co_await static_cast<B *>(this)->io_.read(
                static_cast<B *>(this)->r_stream_.w_splice());
            if (!ret) [[unlikely]] {
                co_return std::unexpected{ret.error()};
            }
            if (ret.value() == 0) {
                break;
            }
            static_cast<B *>(this)->r_stream_.w_increase(ret.value());
        }
        co_return buf.size();
    }

    template <class C>
    [[REMEMBER_CO_AWAIT]]
    auto read_line(C &buf) {
        return read_until(buf, '\n');
    }

private:
    template <bool eof_is_error, class Pred>
        requires std::is_invocable_v<Pred>
    auto real_read(std::span<char> buf, Pred pred) -> zedio::async::Task<Result<std::size_t>> {
        if (static_cast<B *>(this)->r_stream_.capacity() < buf.size_bytes()) {
            auto len = static_cast<B *>(this)->r_stream_.write_to(buf);
            buf = buf.subspan(len, buf.size_bytes() - len);
            co_return co_await static_cast<B *>(this)->io_.read(buf);
        }

        if (pred()) {
            co_return static_cast<B *>(this)->r_stream_.write_to(buf);
        }

        if (static_cast<B *>(this)->r_stream_.w_remaining() < buf.size_bytes()) {
            static_cast<B *>(this)->r_stream_.reset_data();
        }

        assert(static_cast<B *>(this)->r_stream_.w_remaining() >= buf.size_bytes());

        Result<std::size_t> ret;
        while (true) {
            ret = co_await static_cast<B *>(this)->io_.read(
                static_cast<B *>(this)->r_stream_.w_splice());
            if (!ret) [[unlikely]] {
                co_return std::unexpected{ret.error()};
            }
            static_cast<B *>(this)->r_stream_.w_increase(ret.value());

            if constexpr (eof_is_error) {
                if (ret.value() == 0) {
                    co_return std::unexpected{make_zedio_error(Error::UnexpectedEOF)};
                }
                if (pred()) {
                    co_return static_cast<B *>(this)->r_stream_.write_to(buf);
                }
            } else {
                if (pred() || ret.value() == 0) {
                    co_return static_cast<B *>(this)->r_stream_.write_to(buf);
                }
            }
        }
    }
};

} // namespace zedio::io::detail
