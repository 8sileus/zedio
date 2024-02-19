#pragma once

#include "zedio/common/error.hpp"
#include "zedio/fs/builder.hpp"
// C++
#include <filesystem>
// Linux
#include <fcntl.h>

namespace zedio::fs {

class File {
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

    [[nodiscard]] auto static options() -> detail::Builder<File> {
        return detail::Builder<File>{};
    }

    [[nodiscard]] auto static from_fd(int fd) -> File {
        return File{fd};
    }

private:
    int fd_;
};

} // namespace zedio::fs
