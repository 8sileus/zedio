#pragma once

#include "zedio/common/concepts.hpp"
#include "zedio/common/error.hpp"
#include "zedio/fs/builder.hpp"
#include "zedio/fs/impl/impl_async_fsync.hpp"
#include "zedio/fs/impl/impl_async_metadata.hpp"
#include "zedio/io/impl/impl_async_read.hpp"
#include "zedio/io/impl/impl_async_write.hpp"
#include "zedio/io/io.hpp"
// Linux
#include <fcntl.h>

namespace zedio::fs {

class File : public io::detail::Fd,
             public io::detail::ImplAsyncRead<File>,
             public io::detail::ImplAsyncWrite<File>,
             public detail::ImplAsyncFsync<File>,
             public detail::ImplAsyncMetadata<File> {
private:
    friend class detail::Builder<File>;

private:
    explicit File(const int fd)
        : Fd{fd} {}

public:
    template <class T>
        requires constructible_to_char_splice<T>
    [[REMEMBER_CO_AWAIT]]
    auto read_to_end(T &buf) const noexcept -> zedio::async::Task<Result<void>> {
        auto old_len = buf.size();
        {
            auto ret = co_await this->metadata();
            if (!ret) {
                co_return std::unexpected{ret.error()};
            }
            buf.resize(old_len + ret.value().stx_size - lseek64(fd_, 0, SEEK_CUR));
        }
        auto span = std::span<char>{buf}.subspan(old_len);
        auto ret = Result<std::size_t>{};
        while (true) {
            ret = co_await this->read(span);
            if (!ret) [[unlikely]] {
                co_return std::unexpected{ret.error()};
            }
            if (ret.value() == 0) {
                break;
            }
            span = span.subspan(ret.value());
            if (span.empty()) {
                break;
            }
        }
        co_return Result<void>{};
    }

    [[nodiscard]]
    auto seek(off64_t offset, int whence) noexcept {
        ::lseek64(fd_, offset, whence);
    }

public:
    [[REMEMBER_CO_AWAIT]]
    static auto open(std::string_view path) {
        return options().read(true).open(path);
    }

    [[REMEMBER_CO_AWAIT]]
    static auto create(std::string_view path, mode_t permission = 0666) {
        return options().write(true).create(true).truncate(true).permission(permission).open(path);
    }

    [[nodiscard]]
    static auto options() -> detail::Builder<File> {
        return detail::Builder<File>{};
    }
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
static inline auto read_to_end(std::string_view path) -> async::Task<Result<std::string>> {
    auto file = co_await File::open(path);
    if (!file) [[unlikely]] {
        co_return std::unexpected{file.error()};
    }
    std::string res;
    if (auto ret = co_await file.value().read_to_end(res); !ret) [[unlikely]] {
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

[[REMEMBER_CO_AWAIT]]
static inline auto metadata(std::string_view dir_path) {
    return detail::Metadata(dir_path);
}

} // namespace zedio::fs
