#pragma once

#include "zedio/async/operations.hpp"
#include "zedio/common/error.hpp"
#include "zedio/fs/builder.hpp"

// C++
#include <filesystem>
// Linux
#include <fcntl.h>

namespace zedio::fs {

class File {
private:
    class Builder {
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

        auto open(std::string_view path) -> detail::FileBuilderAwaiter<File> {
            return detail::FileBuilderAwaiter<File>(AT_FDCWD, path, flags_, mode_);
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

private:
    File(int fd)
        : fd_{fd} {}

public:
    File(File &&other)
        : fd_{other.fd_} {
        other.fd_ = -1;
    }

    auto operator=(File &&other) -> File & {
        if (fd_ >= 0) {
            // this->close();
        }
        fd_ = other.fd_;
        other.fd_ = -1;
        return *this;
    }

    [[nodiscard]]
    auto write_all(std::span<const char> buf) const noexcept -> async::Task<Result<void>> {
        std::size_t has_written_bytes = 0;
        std::size_t remaining_bytes = buf.size_bytes();
        while (remaining_bytes > 0) {
            auto ret
                = co_await async::write(fd_, buf.data() + has_written_bytes, remaining_bytes, 0);
            if (!ret) [[unlikely]] {
                co_return std::unexpected{ret.error()};
            }
            has_written_bytes += ret.value();
            remaining_bytes -= ret.value();
        }
        co_return Result<void>{};
    }

    auto set_len();

    auto metadata();

    auto set_permissions();

    auto read_to_end();

    auto close() const noexcept {
        return async::close(fd_);
    }

public:
    [[nodiscard]] auto static open(std::string_view path) {
        return options().read(true).open(path);
    }

    [[nodiscard]] auto static create(std::string_view path, mode_t permission = 0666) {
        return options().write(true).create(true).truncate(true).mode(permission).open(path);
    }

    [[nodiscard]] auto static options() -> Builder {
        return Builder{};
    }

    [[nodiscard]] auto static from_fd(int fd) -> File {
        return File{fd};
    }

private:
    int fd_;
};

} // namespace zedio::fs
