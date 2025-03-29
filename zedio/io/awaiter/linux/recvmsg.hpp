#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

namespace detail {

    class RecvMsg : public IORegistrator<RecvMsg> {
    private:
        using Super = IORegistrator<RecvMsg>;

    public:
        RecvMsg(int fd, struct msghdr *msg, unsigned flags)
            : Super{io_uring_prep_recvmsg, fd, msg, flags} {}

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
static inline auto recvmsg(int fd, struct msghdr *msg, unsigned flags) {
    return detail::RecvMsg{fd, msg, flags};
}

} // namespace zedio::io
