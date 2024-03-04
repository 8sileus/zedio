#pragma once

#include "zedio/io/io.hpp"

namespace zedio::fs::detail {

class FileIO : public io::detail::IO {
private:
    explicit FileIO(int fd)
        : IO{fd} {}

public:
    [[REMEMBER_CO_AWAIT]]
    auto fsync(unsigned int fsync_flags) noexcept {
        return io::detail::Fsync{fd_, fsync_flags};
    }

    [[REMEMBER_CO_AWAIT]]
    auto metadata() const noexcept {
        class Statx : public io::detail::IORegistrator<Statx> {
        private:
            using Super = io::detail::IORegistrator<Statx>;

        public:
            Statx(int fd)
                : Super{io_uring_prep_statx,
                        fd,
                        "",
                        AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                        STATX_ALL,
                        &statx_} {}

            auto await_resume() const noexcept -> Result<struct statx> {
                if (this->cb_.result_ >= 0) [[likely]] {
                    return statx_;
                } else {
                    return std::unexpected{make_sys_error(-this->cb_.result_)};
                }
            }

        private:
            struct statx statx_ {};
        };
        return Statx{fd_};
    }

    template <class T>
    [[REMEMBER_CO_AWAIT]]
    auto read_to_end(T &buf) const noexcept -> zedio::async::Task<Result<void>> {
        auto offset = buf.size();
        {
            auto ret = co_await this->metadata();
            if (!ret) {
                co_return std::unexpected{ret.error()};
            }
            buf.resize(offset + ret.value().stx_size);
        }
        auto span = std::span<char>{buf}.subspan(offset);
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

public:
    template <class FileType>
    [[REMEMBER_CO_AWAIT]]
    static auto open(std::string_view path, int flags, mode_t mode) {
        class Open : public io::detail::IORegistrator<Open> {
        private:
            using Super = io::detail::IORegistrator<Open>;

        public:
            Open(int dfd, const char *path, int flags, mode_t mode)
                : Super{io_uring_prep_openat, dfd, path, flags, mode} {}

            Open(const char *path, int flags, mode_t mode)
                : Open{AT_FDCWD, path, flags, mode} {}

            auto await_resume() const noexcept -> Result<FileType> {
                if (this->cb_.result_ >= 0) [[likely]] {
                    return FileType{FileIO{this->cb_.result_}};
                } else {
                    return std::unexpected{make_sys_error(-this->cb_.result_)};
                }
            }
        };
        return Open{path.data(), flags, mode};
    }
};

} // namespace zedio::fs::detail
