#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

class Read : public detail::IORegistrator<Read, decltype(io_uring_prep_read)> {
private:
    using Super = detail::IORegistrator<Read, decltype(io_uring_prep_read)>;

public:
    Read(int fd, void *buf, std::size_t nbytes, uint64_t offset)
        : Super{io_uring_prep_read, fd, buf, nbytes, offset} {}

    Read(int fd, std::span<char> buf, uint64_t offset)
        : Super{io_uring_prep_read, fd, buf.data(), buf.size_bytes(), offset} {}

    auto await_resume() const noexcept -> Result<std::size_t> {
        if (this->cb_.result_ >= 0) [[likely]] {
            return static_cast<std::size_t>(this->cb_.result_);
        } else {
            return std::unexpected{make_sys_error(-this->cb_.result_)};
        }
    }
};

} // namespace zedio::io
