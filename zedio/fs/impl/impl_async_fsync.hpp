#pragma once

#include "zedio/io/awaiter/fsync.hpp"

namespace zedio::fs::detail {

template <class T>
class ImplAsyncFsync {
private:
    using Fsync = io::detail::Fsync;

public:
    [[REMEMBER_CO_AWAIT]]
    auto fsync_all() noexcept -> Fsync {
        return Fsync{static_cast<T *>(this)->fd(), 0};
    }

    [[REMEMBER_CO_AWAIT]]
    auto fsync_data() noexcept -> Fsync {
        return Fsync{static_cast<T *>(this)->fd(), IORING_FSYNC_DATASYNC};
    }

    [[REMEMBER_CO_AWAIT]]
    auto fsync(unsigned fsync_flags) noexcept -> Fsync {
        return Fsync{static_cast<T *>(this)->fd(), fsync_flags};
    }
};

} // namespace zedio::fs::detail