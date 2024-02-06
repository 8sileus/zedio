#pragma once

#include "zed/async/operations.hpp"
#include "zed/common/error.hpp"

// C++
#include <filesystem>

namespace zed::fs {

class File {
private:
    class Builder {
    public:
        auto create(bool on) noexcept -> Builder & {
            if (on) {
                how_.flags |= O_CREAT;
            }
        }

        auto append(bool on) noexcept -> Builder & {
            if (on) {
                how_.flags |= O_APPEND;
            }
        }

        auto write(bool on) noexcept -> Builder & {
            if (on) {
                how_.flags |= O_WRONLY;
            }
        }

        auto read(bool on) noexcept -> Builder & {
            if (on) {
                how_.flags |= O_RDONLY;
            }
        }

        auto truncate(bool on) noexcept -> Builder & {
            if (on) {
                how_.flags |= O_TRUNC;
            }
        }

        auto open(std::string_view path) -> async::Task<Result<File>> {
            auto total_path = std::filesystem::absolute(path);
            auto root_path = total_path.root_path();
            int  dfd = ::open(root_path.c_str(), O_DIRECTORY);
            if (dfd == -1) [[unlikely]] {
                co_return std::unexpected{make_sys_error(errno)};
            }
            auto file_name = total_path.filename();
            auto ret = co_await async::openat(dfd, file_name.c_str(), &how_);
            if (!ret) [[unlikely]] {
                co_return std::unexpected{ret.error()};
            } else {
                co_return File{ret.value()};
            }
        }

    private:
        struct open_how how_ {};
    };

public:
    File(int fd)
        : fd_{fd} {}

    ~File() {
        this->close();
    }

    File(File &&other)
        : fd_{other.fd_} {
        other.fd_ = -1;
    }

    auto operator=(File &&other) -> File & {
        if (fd_ >= 0) {
            this->close();
        }
        fd_ = other.fd_;
        return *this;
    }

    void close() {
        if (fd_ >= 0) {
            ::close(fd_);
        }
    }

public:
    [[nodiscard]] auto static open(std::string_view path) {
        return options().read(true).open(path);
    }

    [[nodiscard]] auto static create(std::string_view path) {
        return options().write(true).create(true).truncate(true).open(path);
    }

private:
    [[nodiscard]] auto static options() -> Builder {
        return Builder{};
    }

private:
    int fd_;
};

} // namespace zed::fs
