#pragma once

#include "zedio/async/awaiter_io.hpp"

namespace zedio::fs::detail {

template <class FileType>
class FileBuilderAwaiter
    : public async::detail::OpenAtAwaiter<async::detail::OPFlag::Distributive> {
public:
    FileBuilderAwaiter(int fd, const std::string_view &path, int flags, mode_t mode)
        : OpenAtAwaiter{fd, path.data(), flags, mode} {}

    auto await_resume() const noexcept -> Result<FileType> {
        auto ret = BaseIOAwaiter::await_resume();
        if (!ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        return FileType{FileType::from_fd(ret.value())};
    }
};

} // namespace zedio::fs::detail
