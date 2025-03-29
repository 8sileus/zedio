#pragma once

#include "zedio/io/buf/buffer.hpp"

namespace zedio::io::detail {

template <class T>
class ImplBufRead {
public:
    [[nodiscard]]
    auto buffer(this T &self) noexcept {
        return self.r_stream_.r_slice();
    }

    [[nodiscard]]
    auto capacity(this const T &self) noexcept {
        return self.r_stream_.capacity();
    }

    void consume(this T &self, std::size_t n) noexcept {
        n = std::min(n, self.r_stream_.r_remaining());
        self.r_stream_.r_increase(n);
    }

    [[REMEMBER_CO_AWAIT]]
    auto fill_buf(this T &self) -> zedio::async::Task<Result<void>> {

        // auto ret
        //     = self.io.read(self.r_stream_.w_slice().data(), self.r_stream_.w_slice().size_bytes());

        auto ret = co_await io::read(self.io_.handle(),
                                     self.r_stream_.w_slice().data(),
                                     self.r_stream_.w_slice().size_bytes(),
                                     -1);
        if (!ret) [[unlikely]] {
            co_return std::unexpected{ret.error()};
        }
        self.r_stream_.w_increase(ret.value());
        co_return Result<void>{};
    }

    [[REMEMBER_CO_AWAIT]]
    auto read(this T &self, std::span<char> buf) {
        return real_read<false>(buf, [this]() -> bool { return self.r_stream_.empty(); });
    }

    [[REMEMBER_CO_AWAIT]]
    auto read_exact(this T &self, std::span<char> buf) {
        return real_read<true>(buf, [this, exact_bytes = buf.size_bytes()]() -> bool {
            return self.r_stream_.r_remaining() >= exact_bytes;
        });
    }

    template <typename C, typename T>
        requires requires(std::string_view s, T s_or_c) {
            { s.find(s_or_c) };
        } && (std::is_same_v<std::string, C> || std::is_same_v<std::vector<char>, C>)
    [[REMEMBER_CO_AWAIT]]
    auto read_until(this T &self, C &buf, T end_flag) -> zedio::async::Task<Result<std::size_t>> {
        Result<std::size_t> ret;
        while (true) {
            if (auto slice = self.r_stream_.find_flag_and_return_slice(end_flag); !slice.empty()) {
                if constexpr (std::is_same_v<std::string, C>) {
                    buf.append(slice.begin(), slice.end());
                } else {
                    buf.insert(buf.end(), slice.begin(), slice.end());
                }
                self.r_stream_.r_increase(slice.size_bytes());
                break;
            } else {
                if constexpr (std::is_same_v<std::string, C>) {
                    buf.append(self.r_stream_.r_slice().begin(), self.r_stream_.r_slice().end());
                } else {
                    buf.insert(buf.end(),
                               self.r_stream_.r_slice().begin(),
                               self.r_stream_.r_slice().end());
                }
                self.r_stream_.reset_pos();
            }

            ret = co_await self.io_.read(self.r_stream_.w_slice());
            if (!ret) [[unlikely]] {
                co_return std::unexpected{ret.error()};
            }
            if (ret.value() == 0) {
                break;
            }
            self.r_stream_.w_increase(ret.value());
        }
        co_return buf.size();
    }

    template <class C>
    [[REMEMBER_CO_AWAIT]]
    auto read_line(this T &self, C &buf) {
        return self.read_until(buf, '\n');
    }

private:
    template <bool eof_is_error, class Pred>
        requires std::is_invocable_v<Pred>
    auto real_read(this T         &self,
                   std::span<char> buf,
                   Pred            pred) -> zedio::async::Task<Result<std::size_t>> {
        if (self.r_stream_.capacity() < buf.size_bytes()) {
            auto len = self.r_stream_.write_to(buf);
            buf = buf.subspan(len, buf.size_bytes() - len);
            if constexpr (eof_is_error) {
                auto ret = co_await self.io_.read_exact(buf);
                if (ret) {
                    co_return len + buf.size_bytes();
                }
            } else {
                auto ret = co_await self.io_.read(buf);
                if (ret) {
                    ret.value() += len;
                }
                co_return ret;
            }
        }

        if (pred()) {
            co_return self.r_stream_.write_to(buf);
        }

        Result<std::size_t> ret;
        while (true) {
            if (self.r_stream_.w_remaining() < buf.size_bytes()) {
                self.r_stream_.reset_data();
            }

            ret = co_await self.io_.read(self.r_stream_.w_slice());

            if (!ret) [[unlikely]] {
                co_return std::unexpected{ret.error()};
            }
            self.r_stream_.w_increase(ret.value());

            if constexpr (eof_is_error) {
                if (ret.value() == 0) {
                    co_return std::unexpected{make_error(Error::UnexpectedEOF)};
                }
                if (pred()) {
                    co_return self.r_stream_.write_to(buf);
                }
            } else {
                if (pred() || ret.value() == 0) {
                    co_return self.r_stream_.write_to(buf);
                }
            }
        }
    }
};

} // namespace zedio::io::detail
