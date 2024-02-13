#pragma once

#include "zed/common/debug.hpp"
#include "zed/common/util/noncopyable.hpp"
#include "zed/net/socket_addr.hpp"
#include "zed/net/tcp_stream.hpp"
// C
#include <cstring>
// C++
#include <expected>
#include <vector>

namespace zed::net {

class TcpSocket;

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
                return std::unexpected{make_zed_error(zed::Error::NullSeq)};
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
    explicit TcpListener(Socket &&sock, std::unique_ptr<AcceptMultishot> &&accept_awaiter)
        : socket_{std::move(sock)}
        , accept_awaiter_{std::move(accept_awaiter)} {
        LOG_TRACE("Build a TcpListener {{fd: {}}}", socket_.fd());
    }

public:
    TcpListener(TcpListener &&other)
        : socket_{std::move(other.socket_)}
        , accept_awaiter_{std::move(other.accept_awaiter_)} {}

    auto operator=(TcpListener &&other) -> TcpListener & {
        socket_ = std::move(other.socket_);
        accept_awaiter_ = std::move(other.accept_awaiter_);
        return *this;
    }

    [[nodiscard]]
    auto accept() noexcept -> AcceptMultishot & {
        return *accept_awaiter_;
    }

    [[nodiscard]]
    auto local_addr() const noexcept {
        return socket_.local_addr();
    }

    [[nodiscard]]
    auto fd() const noexcept {
        return socket_.fd();
    }

    [[nodiscard]]
    auto set_ttl(uint32_t ttl) const noexcept {
        return socket_.set_ttl(ttl);
    }

    [[nodiscard]]
    auto ttl() const noexcept {
        return socket_.ttl();
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
        return build(std::move(sock.value()));
    }

    [[nodiscard]]
    static auto bind(const std::span<SocketAddr> &addresses) -> Result<TcpListener> {
        for (const auto &address : addresses) {
            if (auto ret = bind(address); ret) [[likely]] {
                return ret;
            } else {
                LOG_ERROR("Bind {} failed, error: {}", address.to_string(), ret.error());
            }
        }
        return std::unexpected{make_zed_error(Error::InvalidSockAddrs)};
    }

private:
    [[nodiscard]]
    static auto build(Socket &&sock) -> Result<TcpListener> {
        if (auto ret = AcceptMultishot::create(sock.fd()); !ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        } else {
            return TcpListener{std::move(sock), std::move(ret.value())};
        }
    }

private:
    Socket                           socket_;
    std::unique_ptr<AcceptMultishot> accept_awaiter_;
};

} // namespace zed::net
