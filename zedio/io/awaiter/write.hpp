#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

namespace detail {

    class Write : public IORegistrator<Write> {
    private:
        using Super = IORegistrator<Write>;

    public:
        Write(int fd, const void *buf, unsigned nbytes, __u64 offset)
            : Super{io_uring_prep_write, fd, buf, nbytes, offset} {}

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
auto write(int fd, const void *buf, unsigned nbytes, __u64 offset) {
    return detail::Write(fd, buf, nbytes, offset);
}

} // namespace zedio::io
