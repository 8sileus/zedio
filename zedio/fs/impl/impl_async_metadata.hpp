#pragma once

#include "zedio/io/awaiter/statx.hpp"

namespace zedio::fs::detail {

class Metadata : public io::detail::IORegistrator<Metadata> {
private:
    using Super = io::detail::IORegistrator<Metadata>;

public:
    Metadata(int fd)
        : Super{io_uring_prep_statx,
                fd,
                "",
                AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                STATX_ALL,
                &statx_} {}

    Metadata(std::string_view path)
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

template <class T>
struct ImplAsyncMetadata {
    [[REMEMBER_CO_AWAIT]]
    auto metadata() const noexcept -> Metadata {
        return detail::Metadata{static_cast<const T *>(this)->fd()};
    }
};

} // namespace zedio::fs::detail
