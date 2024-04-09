#pragma once

#include "zedio/io/awaiter/send.hpp"

namespace zedio::socket::detail {

template <class T>
struct ImplStreamWrite {
    auto write(std::span<const char> buf) noexcept {
        return io::detail::Send{static_cast<T *>(this)->fd(),
                                buf.data(),
                                buf.size_bytes(),
                                MSG_NOSIGNAL};
    }

    auto write_zc(std::span<const char> buf) noexcept {
        return io::detail::SendZC{static_cast<T *>(this)->fd(),
                                  buf.data(),
                                  buf.size_bytes(),
                                  MSG_NOSIGNAL,
                                  0};
    }

    template <typename... Ts>
        requires(constructible_to_char_splice<Ts> && ...)
    [[REMEMBER_CO_AWAIT]]
    auto write_vectored(Ts &&...bufs) noexcept {
        constexpr auto N = sizeof...(Ts);

        class WriteVectored : public io::detail::IORegistrator<WriteVectored> {
        private:
            using Super = io::detail::IORegistrator<WriteVectored>;

        public:
            WriteVectored(int fd,Ts&&...bufs)
                : Super{io_uring_prep_sendmsg,fd,&msg_,MSG_NOSIGNAL}
                , iovecs_{ iovec{
                  .iov_base = const_cast<char*>(std::span<const char>(bufs).data()),
                  .iov_len = std::span<const char>(bufs).size_bytes(),
                }...} ,
                msg_{.msg_name =nullptr,
                       .msg_namelen = 0,
                       .msg_iov = iovecs_.data(),
                       .msg_iovlen = N,
                       .msg_control = nullptr,
                       .msg_controllen = 0,
                       .msg_flags = MSG_NOSIGNAL}
                {}

            auto await_resume() const noexcept -> Result<std::size_t> {
                if (this->cb_.result_ >= 0) [[likely]] {
                    return static_cast<std::size_t>(this->cb_.result_);
                } else {
                    return ::std::unexpected{make_sys_error(-this->cb_.result_)};
                }
            }

        private:
            std::array<struct iovec, N> iovecs_;
            struct msghdr               msg_;
        };
        return WriteVectored{static_cast<T *>(this)->fd(), std::forward<Ts>(bufs)...};
    }

    [[REMEMBER_CO_AWAIT]]
    auto write_all(std::span<const char> buf) noexcept -> zedio::async::Task<Result<void>> {
        Result<std::size_t> ret{0uz};
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

} // namespace zedio::socket::detail
