#pragma once

#include "zedio/async/operation.hpp"

namespace zedio::fs::detail {

template <class FileType>
class Builder {
private:
    class Awaiter : public async::detail::OpenAtAwaiter<async::detail::Mode::S> {
    public:
        Awaiter(int fd, const std::string_view &path, int flags, mode_t mode)
            : OpenAtAwaiter{fd, path.data(), flags, mode} {}

        auto await_resume() const noexcept -> Result<FileType> {
            auto ret = OpenAtAwaiter::await_resume();
            if (!ret) [[unlikely]] {
                return std::unexpected{ret.error()};
            }
            return FileType{FileType::from_fd(ret.value())};
        }
    };

public:
    auto read(bool on) noexcept -> Builder & {
        return set_flag(on, O_RDONLY);
    }

    auto write(bool on) noexcept -> Builder & {
        return set_flag(on, O_WRONLY);
    }

    auto append(bool on) noexcept -> Builder & {
        return set_flag(on, O_APPEND);
    }

    auto create(bool on) noexcept -> Builder & {
        return set_flag(on, O_CREAT);
    }

    auto truncate(bool on) noexcept -> Builder & {
        return set_flag(on, O_TRUNC);
    }

    auto mode(mode_t mode) noexcept -> Builder & {
        mode_ = mode;
        return *this;
    }

    auto open(std::string_view path) -> Awaiter {
        return Awaiter(AT_FDCWD, path, flags_, mode_);
    }

private:
    auto set_flag(bool on, uint64_t flag) noexcept -> Builder & {
        if (on) {
            flags_ |= flag;
        } else {
            flags_ &= ~flag;
        }
        return *this;
    }

private:
    uint64_t flags_{0};
    uint64_t mode_{0};
};

} // namespace zedio::fs::detail
