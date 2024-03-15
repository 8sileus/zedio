#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

namespace detail {

    class Send : public IORegistrator<Send> {
    private:
        using Super = IORegistrator<Send>;

    public:
        Send(int sockfd, const void *buf, size_t len, int flags)
            : Super{io_uring_prep_send, sockfd, buf, len, flags} {}

        auto await_resume() const noexcept -> Result<std::size_t> {
            if (this->cb_.result_ >= 0) [[likely]] {
                return static_cast<std::size_t>(this->cb_.result_);
            } else {
                return ::std::unexpected{make_sys_error(-this->cb_.result_)};
            }
        }
    };

    class SendZC : public IORegistrator<SendZC> {
    private:
        using Super = IORegistrator<SendZC>;

    public:
        SendZC(int sockfd, const void *buf, size_t len, int flags, unsigned zc_flags)
            : Super{io_uring_prep_send_zc, sockfd, buf, len, flags, zc_flags} {}

        auto await_resume() const noexcept -> Result<std::size_t> {
            if (this->cb_.result_ >= 0) [[likely]] {
                return static_cast<std::size_t>(this->cb_.result_);
            } else {
                return ::std::unexpected{make_sys_error(-this->cb_.result_)};
            }
        }
    };

} // namespace detail

[[REMEMBER_CO_AWAIT]]
static inline auto send(int sockfd, const void *buf, size_t len, int flags) {
    return detail::Send{sockfd, buf, len, flags};
}

[[REMEMBER_CO_AWAIT]]
static inline auto send_zc(int sockfd, const void *buf, size_t len, int flags, unsigned zc_flags) {
    return detail::SendZC{sockfd, buf, len, flags, zc_flags};
}

} // namespace zedio::io
