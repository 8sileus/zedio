#pragma once

#include "zedio/common/error.hpp"
#include "zedio/fs/builder.hpp"
#include "zedio/fs/io.hpp"
// Linux
#include <fcntl.h>

namespace zedio::fs {

class File {
    friend detail::FileIO;

private:
    File(detail::FileIO &&io)
        : io_{std::move(io)} {}

public:
    [[REMEMBER_CO_AWAIT]]
    auto write_all(std::span<const char> buf) noexcept {
        return io_.write_all(buf);
    }

    [[REMEMBER_CO_AWAIT]]
    auto write(std::span<const char> buf) noexcept {
        return io_.write(buf);
    }

    template <typename... Ts>
    [[REMEMBER_CO_AWAIT]]
    auto write_vectored(Ts &...bufs) noexcept {
        return io_.write_vectored(bufs...);
    }

    [[REMEMBER_CO_AWAIT]]
    auto read(std::span<char> buf) const noexcept {
        return io_.read(buf);
    }

    [[REMEMBER_CO_AWAIT]]
    auto read_to_string(std::string &buf) {
        return io_.read_to_end(buf);
    }

    [[REMEMBER_CO_AWAIT]]
    auto read_to_end(std::vector<char> &buf) {
        return io_.read_to_end(buf);
    }

    [[nodiscard]]
    auto seek(off64_t offset, int whence) noexcept {
        ::lseek64(io_.fd(), offset, whence);
    }

    [[REMEMBER_CO_AWAIT]]
    auto metadata() const noexcept {
        return io_.metadata();
    }

    [[REMEMBER_CO_AWAIT]]
    auto fsync_all() noexcept {
        return io_.fsync(0);
    }

    [[REMEMBER_CO_AWAIT]]
    auto fsync_data() noexcept {
        return io_.fsync(IORING_FSYNC_DATASYNC);
    }

    [[REMEMBER_CO_AWAIT]]
    auto close() noexcept {
        return io_.close();
    }

    [[nodiscard]]
    auto fd() const noexcept {
        return io_.fd();
    }

public:
    [[REMEMBER_CO_AWAIT]] auto static open(std::string_view path) {
        return options().read(true).open(path);
    }

    [[REMEMBER_CO_AWAIT]] auto static create(std::string_view path, mode_t permission = 0666) {
        return options().write(true).create(true).truncate(true).permission(permission).open(path);
    }

    [[nodiscard]] auto static options() -> detail::Builder<File> {
        return detail::Builder<File>{};
    }

private:
    detail::FileIO io_;
};

[[REMEMBER_CO_AWAIT]]
static inline auto read(std::string path) -> async::Task<Result<std::vector<char>>> {
    auto file = co_await File::open(path);
    if (!file) [[unlikely]] {
        co_return std::unexpected{file.error()};
    }
    std::vector<char> res;
    if (auto ret = co_await file.value().read_to_end(res); !ret) [[unlikely]] {
        co_return std::unexpected{ret.error()};
    }
    co_return res;
}

[[REMEMBER_CO_AWAIT]]
static inline auto read_to_string(std::string path) -> async::Task<Result<std::string>> {
    auto file = co_await File::open(path);
    if (!file) [[unlikely]] {
        co_return std::unexpected{file.error()};
    }
    std::string res;
    if (auto ret = co_await file.value().read_to_string(res); !ret) [[unlikely]] {
        co_return std::unexpected{ret.error()};
    }
    co_return res;
}

[[REMEMBER_CO_AWAIT]]
static inline auto rename(std::string_view old_path, std::string_view new_path) {
    return io::rename(old_path.data(), new_path.data());
}

[[REMEMBER_CO_AWAIT]]
static inline auto remove_file(std::string_view path) {
    return io::unlink(path.data(), 0);
}

[[REMEMBER_CO_AWAIT]]
static inline auto hard_link(std::string_view src_path, std::string_view dst_path) {
    return io::link(src_path.data(), dst_path.data(), 0);
}

[[REMEMBER_CO_AWAIT]]
static inline auto sym_link(std::string_view src_path, std::string_view dst_path) {
    return io::symlink(src_path.data(), dst_path.data());
}

} // namespace zedio::fs
