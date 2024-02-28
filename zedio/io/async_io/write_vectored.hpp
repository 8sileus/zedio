#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

struct WriteVectored : public detail::IORegistrator<WriteVectored, decltype(io_uring_prep_writev)> {
private:
    using Super = detail::IORegistrator<WriteVectored, decltype(io_uring_prep_writev)>;

public:
    WriteVectored(int fd, const struct iovec *iovecs, unsigned nr_vecs, __u64 offset)
        : Super{io_uring_prep_writev, fd, iovecs, nr_vecs, offset} {}

    auto await_resume() const noexcept -> Result<std::size_t> {
        if (this->cb_.result_ >= 0) [[likely]] {
            return static_cast<std::size_t>(this->cb_.result_);
        } else {
            return std::unexpected{make_sys_error(-this->cb_.result_)};
        }
    }
};

} // namespace zedio::io