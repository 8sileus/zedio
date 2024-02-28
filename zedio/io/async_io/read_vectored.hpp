#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

class ReadVectored : public detail::IORegistrator<ReadVectored, decltype(io_uring_prep_readv)> {
private:
    using Super = detail::IORegistrator<ReadVectored, decltype(io_uring_prep_readv)>;

public:
    ReadVectored(int fd, const struct iovec *iovecs, unsigned nr_vecs, __u64 offset)
        : Super{io_uring_prep_readv, fd, iovecs, nr_vecs, offset} {}

    auto await_resume() const noexcept -> Result<std::size_t> {
        if (this->cb_.result_ >= 0) [[likely]] {
            return static_cast<std::size_t>(this->cb_.result_);
        } else {
            return std::unexpected{make_sys_error(-this->cb_.result_)};
        }
    }
};

} // namespace zedio::io
