#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

namespace detail {

    class Read : public IORegistrator<Read> {
    private:
        using Super = IORegistrator<Read>;

    public:
        Read(int fd, void *buf, std::size_t nbytes, uint64_t offset)
            : Super{io_uring_prep_read, fd, buf, nbytes, offset} {}

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
auto read(int fd, void *buf, std::size_t nbytes, uint64_t offset) {
    return detail::Read{fd, buf, nbytes, offset};
}

} // namespace zedio::io
