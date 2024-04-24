#pragma once

#include "zedio/common/debug.hpp"
#include "zedio/common/error.hpp"
#include "zedio/io/io.hpp"

namespace zedio::io {

namespace detail {

    class Fd {
    protected:
        explicit Fd(int fd)
            : fd_{fd} {}

        ~Fd() = default;

    public:
        // Delete move
        Fd(Fd &&other) noexcept = delete;
        auto operator=(Fd &&other) noexcept -> Fd & = delete;
        // Delete copy
        Fd(const Fd &other) noexcept = delete;
        auto operator=(const Fd &other) noexcept -> Fd & = delete;

    public:
        [[nodiscard]]
        auto fd() const noexcept {
            return fd_;
        }

    protected:
        int fd_;
    };

    template <typename Derived>
    class AsyncRead {
    public:
        [[REMEMBER_CO_AWAIT]]
        auto read(std::span<char> buf) const noexcept -> async::Task<Result<void>> {
            auto ret = co_await io::read(static_cast<const Derived *>(this)->fd(),
                                         buf.data(),
                                         static_cast<unsigned int>(buf.size_bytes()),
                                         static_cast<std::size_t>(-1));
            if (!ret) [[unlikely]] {
                co_return std::unexpected{ret.error()};
            }
            if (ret.value() == 0) [[unlikely]] {
                co_return std::unexpected{make_zedio_error(Error::UnexpectedEOF)};
            }
            co_return Result<void>{};
        }
    };

    template <typename Derived>
    class AsyncWrite {
    public:
        [[REMEMBER_CO_AWAIT]]
        auto write(std::span<const char> buf) noexcept -> async::Task<Result<void>> {
            Result<std::size_t> ret{0uz};
            while (!buf.empty()) {
                ret = co_await io::write(static_cast<Derived *>(this)->fd(),
                                         buf.data(),
                                         static_cast<unsigned int>(buf.size_bytes()),
                                         static_cast<std::size_t>(-1));
                if (!ret) [[unlikely]] {
                    co_return std::unexpected{ret.error()};
                }
                if (ret.value() == 0) [[unlikely]] {
                    co_return std::unexpected{make_zedio_error(Error::WriteZero)};
                }
                buf = buf.subspan(ret.value(), buf.size_bytes() - ret.value());
            }
            co_return Result<void>{};
        }
    };

    class Stdin : public Fd, public AsyncRead<Stdin> {
    public:
        Stdin()
            : Fd{STDIN_FILENO} {}
    };

    class Stdout : public Fd, public AsyncWrite<Stdout> {
    public:
        Stdout()
            : Fd{STDOUT_FILENO} {}
    };

    class Stderr : public Fd, public AsyncWrite<Stderr> {
    public:
        Stderr()
            : Fd{STDERR_FILENO} {}
    };
} // namespace detail

[[REMEMBER_CO_AWAIT]]
static inline auto input(std::span<char> buf) -> async::Task<Result<void>> {
    co_return co_await util::Singleton<detail::Stdin>::instance().read(buf);
}

template <typename... Args>
[[REMEMBER_CO_AWAIT]]
static inline auto print(std::string_view fmt, Args &&...args) -> async::Task<Result<void>> {
    co_return co_await util::Singleton<detail::Stdout>::instance().write(
        std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...)));
}

template <typename... Args>
[[REMEMBER_CO_AWAIT]]
static inline auto eprint(std::string_view fmt, Args &&...args) -> async::Task<Result<void>> {
    co_return co_await util::Singleton<detail::Stderr>::instance().write(
        std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...)));
}

} // namespace zedio::io
