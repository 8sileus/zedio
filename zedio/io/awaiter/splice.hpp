#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

class Splice : public detail::IORegistrator<Splice, decltype(io_uring_prep_splice)> {
private:
    using Super = detail::IORegistrator<Splice, decltype(io_uring_prep_splice)>;

public:
    Splice(int          fd_in,
           int64_t      off_in,
           int          fd_out,
           int64_t      off_out,
           unsigned int nbytes,
           unsigned int splice_flags)
        : Super{io_uring_prep_splice, fd_in, off_in, fd_out, off_out, nbytes, splice_flags} {}

    auto await_resume() const noexcept -> Result<std::size_t> {
        if (this->cb_.result_ >= 0) [[likely]] {
            return static_cast<std::size_t>(this->cb_.result_);
        } else {
            return std::unexpected{make_sys_error(-this->cb_.result_)};
        }
    }
};

} // namespace zedio::io
