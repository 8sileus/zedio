#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

class Recv : public detail::IORegistrator<Recv, decltype(io_uring_prep_recv)> {
private:
    using Super = detail::IORegistrator<Recv, decltype(io_uring_prep_recv)>;

public:
    Recv(int sockfd, void *buf, size_t len, int flags)
        : Super{io_uring_prep_recv, sockfd, buf, len, flags} {}

    Recv(int sockfd, std::span<char> buf, int flags)
        : Super{io_uring_prep_recv, sockfd, buf.data(), buf.size_bytes(), flags} {}

    auto await_resume() const noexcept -> Result<std::size_t> {
        if (this->cb_.result_ >= 0) [[likely]] {
            return static_cast<std::size_t>(this->cb_.result_);
        } else {
            return std::unexpected{make_sys_error(-this->cb_.result_)};
        }
    }
};

} // namespace zedio::io
