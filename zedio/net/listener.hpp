#pragma once

#include "zedio/common/concepts.hpp"
#include "zedio/io/io.hpp"

namespace zedio::net::detail {

template <class Listener, class Stream, class Addr>
class BaseListener {
protected:
    using IO = zedio::io::IO;

    explicit BaseListener(IO &&io)
        : io_{std::move(io)} {}

public:
    BaseListener(BaseListener &&other) = default;
    auto operator=(BaseListener &&other) -> BaseListener & = default;

    [[nodiscard]]
    auto accept() const noexcept {
        class Awaiter : public io::Accept {
        public:
            Awaiter(int fd)
                : Accept{fd, reinterpret_cast<struct sockaddr *>(&addr_), &length_, SOCK_NONBLOCK} {
            }

            auto await_resume() const noexcept -> Result<std::pair<Stream, Addr>> {
                if (this->cb_.result_ >= 0) [[likely]] {
                    return std::make_pair(Stream{IO::from_fd(cb_.result_)}, addr_);
                } else {
                    return std::unexpected{make_sys_error(-this->cb_.result_)};
                }
            }

        private:
            Addr      addr_{};
            socklen_t length_{sizeof(Addr)};
        };
        return Awaiter{io_.fd()};
    }

    [[nodiscard]]
    auto local_addr() const noexcept {
        return io_.local_addr<Addr>();
    }

    [[nodiscard]]
    auto fd() const noexcept {
        return io_.fd();
    }

    [[nodiscard]]
    auto close() noexcept {
        return io_.close();
    }

public:
    [[nodiscard]]
    static auto bind(const Addr &addr) -> Result<Listener> {
        auto io = IO::socket(addr.family(), SOCK_STREAM, 0);
        if (!io) [[unlikely]] {
            return std::unexpected{io.error()};
        }
        if (auto ret = io.value().bind(addr); !ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        if (auto ret = io.value().listen(SOMAXCONN); !ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        return Listener{std::move(io.value())};
    }

    [[nodiscard]]
    static auto bind(const std::span<Addr> &addresses) -> Result<Listener> {
        for (const auto &address : addresses) {
            if (auto ret = bind(address); ret) [[likely]] {
                return ret;
            } else {
                LOG_ERROR("Bind {} failed, error: {}", address.to_string(), ret.error().message());
            }
        }
        return std::unexpected{make_zedio_error(Error::InvalidInput)};
    }

protected:
    IO io_;
};

} // namespace zedio::net::detail
