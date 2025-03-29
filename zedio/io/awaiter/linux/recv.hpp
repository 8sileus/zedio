#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

namespace detail {

    class Recv : public IORegistrator<Recv> {
    private:
        using Super = IORegistrator<Recv>;

    public:
        Recv(int sockfd, void *buf, size_t len, int flags)
            : Super{io_uring_prep_recv, sockfd, buf, len, flags} {}

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
static inline auto recv(int sockfd, void *buf, size_t len, int flags) {
    return detail::Recv{sockfd, buf, len, flags};
}

} // namespace zedio::io
