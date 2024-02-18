#pragma once

#include "zedio/async/operations.hpp"
#include "zedio/common/error.hpp"

// C++
#include <filesystem>

namespace zedio::fs {

class File {
private:
    class Builder {
    public:
        auto create(bool on) noexcept {
            return set_flag(on, O_CREAT);
        }

        auto append(bool on) noexcept {
            return set_flag(on, O_APPEND);
        }

        auto write(bool on) noexcept {
            return set_flag(on, O_WRONLY);
        }

        auto read(bool on) noexcept {
            return set_flag(on, O_RDONLY);
        }

        auto truncate(bool on) noexcept {
            return set_flag(on, O_TRUNC);
        }

        auto open(std::string path) -> async::Task<Result<File>> {
            // std::filesystem::path path{file_name};
            // auto root_path = path.root_path();
            auto ret = co_await async::openat2(AT_FDCWD, path.data(), &how_);
            if (!ret) [[unlikely]] {
                LOG_ERROR("{}", ret.error().value());
                co_return std::unexpected{ret.error()};
            } else {
                co_return File{ret.value()};
            }
        }

    private:
        auto set_flag(bool on, uint64_t flag) noexcept -> Builder & {
            if (on) {
                how_.flags |= flag;
            } else {
                how_.flags &= ~flag;
            }
            return *this;
        }

    private:
        struct open_how how_ {};
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
    auto write_all(std::span<const char> buf) const noexcept
        -> async::Task<Result<void>> {
        std::size_t has_written_bytes = 0;
        std::size_t remaining_bytes = buf.size_bytes();
        while (remaining_bytes > 0) {
            auto ret = co_await async::write(
                fd_, buf.data() + has_written_bytes, remaining_bytes, 0);
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
        return options().read(true).open(std::string{path});
    }

    [[nodiscard]] auto static create(std::string_view path) {
        return options().write(true).create(true).truncate(true).open(
            std::string{path});
    }

    [[nodiscard]] auto static options() -> Builder {
        return Builder{};
    }

private:
    int fd_;
};

} // namespace zedio::fs
