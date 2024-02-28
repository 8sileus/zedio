#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

class Send : public detail::IORegistrator<Send, decltype(io_uring_prep_send)> {
private:
    using Super = detail::IORegistrator<Send, decltype(io_uring_prep_send)>;

public:
    Send(int sockfd, const void *buf, size_t len, int flags)
        : Super{io_uring_prep_send, sockfd, buf, len, flags} {}

    Send(int sockfd, std::span<const char> buf, int flags)
        : Super{io_uring_prep_send, sockfd, buf.data(), buf.size_bytes(), flags} {}

    auto await_resume() const noexcept -> Result<std::size_t> {
        if (this->cb_.result_ >= 0) [[likely]] {
            return static_cast<std::size_t>(this->cb_.result_);
        } else {
            return std::unexpected{make_sys_error(-this->cb_.result_)};
        }
    }
};

} // namespace zedio::io
