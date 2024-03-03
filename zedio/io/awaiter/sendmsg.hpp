#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

namespace detail {

    class SendMsg : public IORegistrator<SendMsg> {
    private:
        using Super = IORegistrator<SendMsg>;

    public:
        SendMsg(int fd, const struct msghdr *msg, unsigned flags)
            : Super{io_uring_prep_sendmsg, fd, msg, flags} {}

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
auto sendmsg(int fd, const struct msghdr *msg, unsigned flags) {
    return detail::SendMsg{fd, msg, flags};
}

} // namespace zedio::io
