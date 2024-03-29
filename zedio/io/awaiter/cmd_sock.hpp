#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

namespace detail {

    class CmdSock : public IORegistrator<CmdSock> {
    private:
        using Super = IORegistrator<CmdSock>;

    public:
        CmdSock(int cmd_op, int fd, int level, int optname, void *optval, int optlen)
            : Super{io_uring_prep_cmd_sock, cmd_op, fd, level, optname, optval, optlen} {}

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
static inline auto getsockopt(int fd, int level, int optname, void *optval, int optlen) {
    return detail::CmdSock{SOCKET_URING_OP_GETSOCKOPT, fd, level, optname, optval, optlen};
}

[[REMEMBER_CO_AWAIT]]
static inline auto setsockopt(int fd, int level, int optname, void *optval, int optlen) {
    return detail::CmdSock{SOCKET_URING_OP_SETSOCKOPT, fd, level, optname, optval, optlen};
}

[[REMEMBER_CO_AWAIT]]
static inline auto cmdsock(int cmd_op, int fd, int level, int optname, void *optval, int optlen) {
    return detail::CmdSock{cmd_op, fd, level, optname, optval, optlen};
}

} // namespace zedio::io
