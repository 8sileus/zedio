#pragma once

#include "zedio/io/io.hpp"

namespace zedio::io::detail {

template <class T>
struct ImplAsyncWrite {
    [[REMEMBER_CO_AWAIT]]
    auto write(std::span<const char> buf) noexcept {
        return Write{static_cast<T *>(this)->fd(),
                     buf.data(),
                     static_cast<unsigned int>(buf.size_bytes()),
                     static_cast<std::size_t>(-1)};
    }

    template <typename... Ts>
        requires(constructible_to_char_slice<Ts> && ...)
    [[REMEMBER_CO_AWAIT]]
    auto write_vectored(Ts &&...bufs) noexcept {
        constexpr std::size_t N = sizeof...(Ts);

        class WriteVectored : public IORegistrator<WriteVectored> {
        private:
            using Super = IORegistrator<WriteVectored>;

        public:
            WriteVectored(int fd,Ts&&...bufs)
                : Super{io_uring_prep_writev,fd, nullptr, N, static_cast<std::size_t>(-1)}
                , iovecs_{ iovec{
                  .iov_base = const_cast<char*>(std::span<const char>(bufs).data()),
                  .iov_len = std::span<const char>(bufs).size_bytes(),
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
        return WriteVectored{static_cast<T *>(this)->fd(), std::forward<Ts>(bufs)...};
    }

    [[REMEMBER_CO_AWAIT]]
    auto write_all(std::span<const char> buf) noexcept -> zedio::async::Task<Result<void>> {
        Result<std::size_t> ret{0};
        while (!buf.empty()) {
            ret = co_await this->write(buf);
            if (!ret) [[unlikely]] {
                co_return std::unexpected{ret.error()};
            }
            if (ret.value() == 0) [[unlikely]] {
                co_return std::unexpected{make_zedio_error(Error::WriteZero)};
            }
            buf = buf.subspan(ret.value(), buf.size_bytes() - ret.value());
        }
        co_return Result<void>{};
    }
};

} // namespace zedio::io::detail
