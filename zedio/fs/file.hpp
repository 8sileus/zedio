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

    ~File() {
        if (fd_ >= 0) {
            // TODO register close
        }
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

    auto read_to_end(std::string &s) {
        // TODO get file size and reserve size
        // TODO register link io
    }

    auto read_to_end(std::vector<char> &s){
        // TODO get file size and reserve size
        // TODO register link io
    }

    auto set_len();

    auto set_permissions();

    [[nodiscard]]
    auto metadata() {
        struct Awaiter : public async::statx<> {
            Awaiter(int fd)
                : async::statx<>{fd, nullptr, AT_EMPTY_PATH, STATX_ALL, &statx_} {}

            auto await_resume() const noexcept -> Result<struct statx> {
                if (auto ret = async::statx<>::await_resume(); !ret) [[unlikely]] {
                    return std::unexpected{ret.error()};
                }
                return statx_;
            }

            struct statx statx_;
        };

        return Awaiter{fd_};
    }

    [[nodiscard]]
    auto fsync(bool sync_metadata = true) const noexcept {
        auto flag{sync_metadata ? 0 : IORING_FSYNC_DATASYNC};
        return async::fsync(fd_, flag);
    }

    [[nodiscard]]
    auto close() noexcept {
        auto fd = fd_;
        fd_ = -1;
        return async::close(fd);
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
