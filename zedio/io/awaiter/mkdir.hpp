#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

class MkDir : public detail::IORegistrator<MkDir, decltype(io_uring_prep_mkdirat)> {
private:
    using Super = detail::IORegistrator<MkDir, decltype(io_uring_prep_mkdirat)>;

public:
    MkDir(int dfd, const char *path, mode_t mode)
        : Super{io_uring_prep_mkdirat, dfd, path, mode} {}

    MkDir(const char *path, mode_t mode)
        : MkDir{AT_FDCWD, path, mode} {}

    auto await_resume() const noexcept -> Result<void> {
        if (this->cb_.result_ >= 0) [[likely]] {
            return {};
        } else {
            return std::unexpected{make_sys_error(-this->cb_.result_)};
        }
    }
};

} // namespace zedio::io
