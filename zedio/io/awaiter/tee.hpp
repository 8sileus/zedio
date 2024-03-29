#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

namespace detail {

    class Tee : public IORegistrator<Tee> {
    private:
        using Super = IORegistrator<Tee>;

    public:
        Tee(int fd_in, int fd_out, unsigned int nbytes, unsigned int splice_flags)
            : Super{io_uring_prep_tee, fd_in, fd_out, nbytes, splice_flags} {}

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
static inline auto tee(int fd_in, int fd_out, unsigned int nbytes, unsigned int splice_flags) {
    return detail::Tee{fd_in, fd_out, nbytes, splice_flags};
}

} // namespace zedio::io
