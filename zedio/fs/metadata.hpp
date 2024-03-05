#pragma once

#include "zedio/io/io.hpp"

namespace zedio::fs {

namespace detail {

    class GetMetaData : public io::detail::IORegistrator<GetMetaData> {
    private:
        using Super = io::detail::IORegistrator<GetMetaData>;

    public:
        GetMetaData(int fd)
            : Super{io_uring_prep_statx,
                    fd,
                    "",
                    AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                    STATX_ALL,
                    &statx_} {}

        GetMetaData(std::string_view path)
            : Super(io_uring_prep_statx,
                    AT_FDCWD,
                    path.data(),
                    AT_STATX_SYNC_AS_STAT,
                    STATX_ALL,
                    &statx_) {}

        auto await_resume() const noexcept -> Result<struct statx> {
            if (this->cb_.result_ >= 0) [[likely]] {
                return statx_;
            } else {
                return std::unexpected{make_sys_error(-this->cb_.result_)};
            }
        }

    private:
        struct statx statx_ {};
    };

} // namespace detail

[[REMEMBER_CO_AWAIT]]
static inline auto metadata(std::string_view dir_path) {
    return detail::GetMetaData(dir_path);
}

} // namespace zedio::fs
