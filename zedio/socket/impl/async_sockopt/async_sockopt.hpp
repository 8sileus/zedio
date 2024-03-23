#pragma once

#include "zedio/io/awaiter/cmd_sock.hpp"
// Linux
#include <netinet/in.h>
#include <netinet/tcp.h>

namespace zedio::socket::detail {

template <typename T>
class SetSockOpt : public io::detail::IORegistrator<SetSockOpt<T>> {
private:
    using Super = io::detail::IORegistrator<SetSockOpt<T>>;

public:
    SetSockOpt(int fd, int level, int optname, T optval)
        : Super{io_uring_prep_cmd_sock,
                SOCKET_URING_OP_SETSOCKOPT,
                fd,
                level,
                optname,
                &optval_,
                sizeof(T)}
        , optval_{optval} {}

    auto await_resume() const noexcept -> Result<void> {
        if (this->cb_.result_ >= 0) [[likely]] {
            return {};
        } else {
            return ::std::unexpected{make_sys_error(-this->cb_.result_)};
        }
    }

private:
    T optval_;
};

template <typename F, typename T>
    requires std::is_invocable_v<F, T>
class GetSockOpt : public io::detail::IORegistrator<GetSockOpt<F, T>> {
private:
    using Super = io::detail::IORegistrator<GetSockOpt<F, T>>;

public:
    GetSockOpt(int fd, int level, int optname, F &&f)
        : Super{io_uring_prep_cmd_sock,
                SOCKET_URING_OP_GETSOCKOPT,
                fd,
                level,
                optname,
                &optval_,
                sizeof(T)}
        , f_{std::forward<F>(f)} {}

    auto await_resume() const noexcept -> Result<std::invoke_result_t<F, T>> {
        if (this->cb_.result_ >= 0) [[likely]] {
            return f_(std::move(optval_));
        } else {
            return ::std::unexpected{make_sys_error(-this->cb_.result_)};
        }
    }

private:
    T optval_{};
    F f_{};
};

template <typename T>
static inline auto setsockopt(int fd, int level, int optname, T optval) {
    return SetSockOpt<T>{fd, level, optname, optval};
}

template <typename T, typename F>
static inline auto getsockopt(int fd, int level, int optname, F &&f) {
    return GetSockOpt<F, T>(fd, level, optname, std::forward<F>(f));
}

} // namespace zedio::socket::detail
