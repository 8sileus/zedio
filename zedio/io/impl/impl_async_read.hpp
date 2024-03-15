#pragma once

#include "zedio/io/io.hpp"

namespace zedio::io::detail {

template <class T>
struct ImplAsyncRead {
    [[REMEMBER_CO_AWAIT]]
    auto read(std::span<char> buf) const noexcept {
        return Read{static_cast<const T *>(this)->fd(),
                    buf.data(),
                    static_cast<unsigned int>(buf.size_bytes()),
                    static_cast<std::size_t>(-1)};
    }

    template <typename... Ts>
        requires(constructible_to_char_splice<Ts> && ...)
    [[REMEMBER_CO_AWAIT]]
    auto read_vectored(Ts &&...bufs) const noexcept {
        constexpr std::size_t N = sizeof...(Ts);

        class ReadVectored : public IORegistrator<ReadVectored> {
        private:
            using Super = IORegistrator<ReadVectored>;

        public:
            ReadVectored(int fd,Ts&...bufs)
                : Super{io_uring_prep_readv,fd, nullptr,N,static_cast<std::size_t>(-1)}
                , iovecs_{ iovec{
                  .iov_base = std::span<char>(bufs).data(),
                  .iov_len = std::span<char>(bufs).size_bytes(),
                }...} {
                this->sqe_->addr = reinterpret_cast<unsigned long long>(iovecs_.data());
            }

            auto await_resume() const noexcept -> Result<std::size_t> {
                if (this->cb_.result_ >= 0) [[likely]] {
                    return static_cast<std::size_t>(this->cb_.result_);
                } else {
                    return ::std::unexpected{make_sys_error(-this->cb_.result_)};
                }
            }

        private:
            std::array<struct iovec, N> iovecs_;
        };
        return ReadVectored{static_cast<const T *>(this)->fd(), std::forward<Ts>(bufs)...};
    }

    [[REMEMBER_CO_AWAIT]]
    auto read_exact(std::span<char> buf) const noexcept -> zedio::async::Task<Result<void>> {
        Result<std::size_t> ret{0uz};
        while (!buf.empty()) {
            ret = co_await this->read(buf);
            if (!ret) [[unlikely]] {
                co_return std::unexpected{ret.error()};
            }
            if (ret.value() == 0) {
                co_return std::unexpected{make_zedio_error(Error::UnexpectedEOF)};
            }
            buf = buf.subspan(ret.value(), buf.size_bytes() - ret.value());
        }
        co_return Result<void>{};
    }
};

} // namespace zedio::io::detail