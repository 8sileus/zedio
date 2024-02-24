#pragma once

#include "zedio/async/io.hpp"
#include "zedio/common/error.hpp"
#include "zedio/fs/builder.hpp"
// C++
#include <filesystem>
// Linux
#include <fcntl.h>

namespace zedio::fs {

class File {
    using IO = zedio::async::detail::IO;
    friend IO;

private:
    File(IO &&io)
        : io_{std::move(io)} {}

public:
    File(File &&other) noexcept = default;
    auto operator=(File &&other) -> File & = default;

    [[nodiscard]]
    auto write_all(std::span<const char> buf) const noexcept {
        return io_.write_all(buf);
    }

    [[nodiscard]]
    auto write(std::span<const char> buf) const noexcept {
        return io_.write(buf);
    }

    template <typename... Ts>
    [[nodiscard]]
    auto write_vectored(Ts &...bufs) const noexcept {
        return io_.write_vectored(bufs...);
    }

    [[nodiscard]]
    auto read(std::span<char> buf) const noexcept {
        return io_.read(buf);
    }

    auto read_to_string(std::string &buf) {
        return io_.read_to_end(buf);
    }

    template <typename T>
    auto read_to_end(std::vector<T> &buf) {
        return io_.read_to_end(buf);
    }

    void seek(off64_t offset, int whence) {
        ::lseek64(io_.fd(), offset, whence);
    }

    [[nodiscard]]
    auto metadata() {
        return io_.metadata();
    }

    [[nodiscard]]
    auto fsync_all() const noexcept {
        return io_.fsync(0);
    }

    [[nodiscard]]
    auto fsync_data() const noexcept {
        return io_.fsync(IORING_FSYNC_DATASYNC);
    }

    [[nodiscard]]
    auto close() noexcept {
        return io_.close();
    }

    [[nodiscard]]
    auto fd() const noexcept {
        return io_.fd();
    }

public:
    [[nodiscard]] auto static open(std::string_view path) {
        return options().read(true).open(path);
    }

    [[nodiscard]] auto static create(std::string_view path, mode_t permission = 0666) {
        return options().write(true).create(true).truncate(true).permission(permission).open(path);
    }

    [[nodiscard]] auto static options() -> detail::Builder<File> {
        return detail::Builder<File>{};
    }

private:
    IO io_;
};

} // namespace zedio::fs
