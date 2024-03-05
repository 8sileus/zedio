#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

namespace detail {

    struct WriteV : public IORegistrator<WriteV> {
    private:
        using Super = IORegistrator<WriteV>;

    public:
        WriteV(int fd, const struct iovec *iovecs, unsigned nr_vecs, __u64 offset, int flags = 0)
            : Super{io_uring_prep_writev2, fd, iovecs, nr_vecs, offset, flags} {}

        auto await_resume() const noexcept -> Result<std::size_t> {
            if (this->cb_.result_ >= 0) [[likely]] {
                return static_cast<std::size_t>(this->cb_.result_);
            } else {
                return ::std::unexpected{make_sys_error(-this->cb_.result_)};
            }
        }
    };

    template <typename... Ts>
    class WriteVectored : public IORegistrator<WriteVectored<Ts...>> {
    private:
        using Super = IORegistrator<WriteVectored<Ts...>>;
        constexpr static auto N = sizeof...(Ts);

    public:
        WriteVectored(int fd,Ts&...bufs)
                : Super{io_uring_prep_write,fd, &iovecs_.data(), N, static_cast<std::size_t>(-1)}
                , iovecs_{ iovec{
                  .iov_base = std::span<char>(bufs).data(),
                  .iov_len = std::span<char>(bufs).size_bytes(),
                }...} {}

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

} // namespace detail

[[REMEMBER_CO_AWAIT]]
static inline auto
writev(int fd, const struct iovec *iovecs, unsigned nr_vecs, __u64 offset, int flags = 0) {
    return detail::WriteV{fd, iovecs, nr_vecs, offset, flags};
}

} // namespace zedio::io
