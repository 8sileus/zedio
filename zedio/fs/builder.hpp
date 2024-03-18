#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::fs::detail {

template <class F>
class Builder {
public:
    auto read(bool on) noexcept -> Builder & {
        read_ = on;
        return *this;
    }

    auto write(bool on) noexcept -> Builder & {
        write_ = on;
        return *this;
    }

    auto append(bool on) noexcept -> Builder & {
        append_ = on;
        return *this;
    }

    auto create(bool on) noexcept -> Builder & {
        create_ = on;
        return *this;
    }

    auto truncate(bool on) noexcept -> Builder & {
        truncate_ = on;
        return *this;
    }

    auto permission(mode_t permission) noexcept -> Builder & {
        permission_ = permission;
        return *this;
    }

    auto custom_flags(int flags) noexcept -> Builder & {
        flags_ |= flags;
    }

    [[REMEMBER_CO_AWAIT]]
    auto open(std::string_view path) {
        adjust_flags();

        class Open : public io::detail::IORegistrator<Open> {
        private:
            using Super = io::detail::IORegistrator<Open>;

        public:
            Open(std::string_view path, int flags, mode_t mode)
                : Super{io_uring_prep_openat, AT_FDCWD, path.data(), flags, mode} {}

            auto await_resume() const noexcept -> Result<F> {
                if (this->cb_.result_ >= 0) [[likely]] {
                    return F{this->cb_.result_};
                } else {
                    return std::unexpected{make_sys_error(-this->cb_.result_)};
                }
            }
        };
        return Open(path, flags_, permission_);
    }

private:
    void adjust_flags() {
        if (read_ && !write_ && !append_) {
            flags_ |= O_RDONLY;
        } else if (!read_ && write_ && !append_) {
            flags_ |= O_WRONLY;
        } else if (read_ && write_ && !append_) {
            flags_ |= O_RDWR;
        } else if (read_ && append_) {
            flags_ |= O_RDWR | O_APPEND;
        } else if (append_) {
            flags_ |= O_WRONLY | O_APPEND;
        } else {
            flags_ = -1;
        }

        if (truncate_) {
            flags_ |= O_TRUNC;
        }
        if (create_) {
            flags_ |= O_CREAT;
        }
    }

private:
    bool read_{false};
    bool write_{false};
    bool append_{false};
    bool create_{false};
    bool truncate_{false};

    int    flags_{0};
    mode_t permission_{0};
};

} // namespace zedio::fs::detail
