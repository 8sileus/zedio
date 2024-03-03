#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

namespace detail {

    class MkDir : public IORegistrator<MkDir> {
    private:
        using Super = IORegistrator<MkDir>;

    public:
        MkDir(int dfd, const char *path, mode_t mode)
            : Super{io_uring_prep_mkdirat, dfd, path, mode} {}

        MkDir(const char *path, mode_t mode)
            : MkDir{AT_FDCWD, path, mode} {}

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
auto mkdir(const char *path, mode_t mode) {
    return detail::MkDir{path, mode};
}

[[REMEMBER_CO_AWAIT]]
auto mkdirat(int dfd, const char *path, mode_t mode) {
    return detail::MkDir{dfd, path, mode};
}

} // namespace zedio::io
