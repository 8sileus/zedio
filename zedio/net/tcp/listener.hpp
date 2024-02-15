#pragma once

#include "zedio/common/debug.hpp"
#include "zedio/common/util/noncopyable.hpp"
#include "zedio/net/tcp/stream.hpp"
// C
#include <cstring>
// C++
#include <expected>
#include <vector>

namespace zedio::net {

class TcpListener : util::Noncopyable {
    friend class TcpSocket;

private:
    class AcceptMultishot {
    public:
        AcceptMultishot()
            : data_{static_cast<int>(async::detail::OPFlag::Exclusive)} {}

        ~AcceptMultishot() {
            auto sqe = async::detail::t_poller->get_sqe();
            io_uring_prep_cancel(sqe, this, 0);
            io_uring_sqe_set_data(sqe, nullptr);
            async::detail::t_poller->submit();
        }

        constexpr auto await_ready() const noexcept -> bool {
            return false;
        }

        auto await_suspend(std::coroutine_handle<> handle) -> std::coroutine_handle<> {
            data_.handle_ = std::move(handle);
            return std::noop_coroutine();
        }

        auto await_resume() const noexcept -> Result<std::pair<TcpStream, SocketAddr>> {
            if (data_.is_good_result()) [[likely]] {
                return std::make_pair(
                    TcpStream{Socket::from_fd(data_.result())},
                    SocketAddr{reinterpret_cast<const sockaddr *>(&addr_), addrlen_});
            }
            assert(data_.is_bad_result());
            return std::unexpected{make_sys_error(-data_.result())};
        }

    public:
        [[nodiscard]]
        static auto create(int fd) -> Result<std::unique_ptr<AcceptMultishot>> {
            auto accept_awaiter = std::make_unique<AcceptMultishot>();
            auto sqe = async::detail::t_poller->get_sqe();
            if (sqe == nullptr) [[unlikely]] {
                return std::unexpected{make_zedio_error(zedio::Error::NullSeq)};
            }
            io_uring_prep_multishot_accept(sqe, fd,
                                           reinterpret_cast<sockaddr *>(&accept_awaiter->addr_),
                                           &accept_awaiter->addrlen_, SOCK_NONBLOCK);
            io_uring_sqe_set_data(sqe, &accept_awaiter->data_);
            if (auto ret = async::detail::t_poller->submit(); !ret) [[unlikely]] {
                return std::unexpected{ret.error()};
            } else {
                return accept_awaiter;
            }
        }

    private:
        async::detail::BaseIOAwaiterData data_;
        sockaddr_in6                     addr_{};
        socklen_t                        addrlen_{0};
    };

private:
    explicit TcpListener(Socket &&sock)
        : io_{std::move(sock)} {
        LOG_TRACE("Build a TcpListener {{fd: {}}}", io_.fd());
    }

public:
    TcpListener(TcpListener &&other)
        : io_{std::move(other.io_)} {}

    [[nodiscard]]
    auto accept() const noexcept -> async::Task<Result<std::pair<TcpStream, SocketAddr>>> {
        sockaddr_in6 addr{};
        socklen_t    addrlen{0};
        auto ret = co_await async::accept(io_.fd(), reinterpret_cast<struct sockaddr *>(&addr),
                                          &addrlen);
        if (!ret) [[unlikely]] {
            co_return std::unexpected{ret.error()};
        }
        co_return std::make_pair(TcpStream{Socket::from_fd(ret.value())},
                                 SocketAddr{reinterpret_cast<const sockaddr *>(&addr), addrlen});
    }

    [[nodiscard]]
    auto local_addr() const noexcept {
        return io_.local_addr<SocketAddr>();
    }

    [[nodiscard]]
    auto fd() const noexcept {
        return io_.fd();
    }

    [[nodiscard]]
    auto set_ttl(uint32_t ttl) const noexcept {
        return io_.set_ttl(ttl);
    }

    [[nodiscard]]
    auto ttl() const noexcept {
        return io_.ttl();
    }

public:
    [[nodiscard]]
    static auto bind(const SocketAddr &address) -> Result<TcpListener> {
        auto sock = Socket::build(address.family(), SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
        if (!sock) [[unlikely]] {
            return std::unexpected{sock.error()};
        }
        if (auto ret = sock.value().bind(address); !ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        if (auto ret = sock.value().listen(SOMAXCONN); !ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        return TcpListener{std::move(sock.value())};
    }

    [[nodiscard]]
    static auto bind(const std::span<SocketAddr> &addresses) -> Result<TcpListener> {
        for (const auto &address : addresses) {
            if (auto ret = bind(address); ret) [[likely]] {
                return ret;
            } else {
                LOG_ERROR("Bind {} failed, error: {}", address.to_string(), ret.error().message());
            }
        }
        return std::unexpected{make_zedio_error(Error::InvalidInput)};
    }

private:
    // [[nodiscard]]
    // static auto build(Socket &&sock) -> Result<TcpListener> {
    //     if (auto ret = AcceptMultishot::create(sock.fd()); !ret) [[unlikely]] {
    //         return std::unexpected{ret.error()};
    //     } else {
    //         return TcpListener{std::move(sock), std::move(ret.value())};
    //     }
    // }

private:
    Socket io_;
    // std::unique_ptr<AcceptMultishot> accept_awaiter_;
};

} // namespace zedio::net
