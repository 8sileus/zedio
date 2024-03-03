#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

namespace detail {

    class FAdvise : public IORegistrator<FAdvise> {
    private:
        using Super = IORegistrator<FAdvise>;

    public:
        FAdvise(int fd, __u64 offset, off_t len, int advice)
            : Super{io_uring_prep_fadvise, fd, offset, len, advice} {}

        auto await_resume() const noexcept -> Result<void> {
            if (this->cb_.result_ >= 0) [[likely]] {
                return {};
            } else {
                return ::std::unexpected{make_sys_error(-this->cb_.result_)};
            }
        }
    };

} // namespace detail

[[REMEMBER_CO_AWAIT]]
auto fadvise(int fd, __u64 offset, off_t len, int advice) {
    return detail::FAdvise{fd, offset, len, advice};
}

} // namespace zedio::io
