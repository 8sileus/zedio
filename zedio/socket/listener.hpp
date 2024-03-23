#pragma once

#include "zedio/io/io.hpp"
#include "zedio/socket/impl/impl_local_addr.hpp"

namespace zedio::socket::detail {

template <class Listener, class Stream, class Addr>
class BaseListener : public io::detail::Fd,
                     public ImplLocalAddr<BaseListener<Listener, Stream, Addr>, Addr> {
protected:
    explicit BaseListener(const int fd)
        : Fd{fd} {}

public:
    [[REMEMBER_CO_AWAIT]]
    auto accept() const noexcept {
        class Accept : public io::detail::IORegistrator<Accept> {
            using Super = io::detail::IORegistrator<Accept>;

        public:
            Accept(int fd)
                : Super{io_uring_prep_accept,
                        fd,
                        reinterpret_cast<struct sockaddr *>(&addr_),
                        &length_,
                        SOCK_NONBLOCK} {}

            auto await_resume() const noexcept -> Result<std::pair<Stream, Addr>> {
                if (this->cb_.result_ >= 0) [[likely]] {
                    return std::make_pair(Stream{this->cb_.result_}, addr_);
                } else {
                    return std::unexpected{make_sys_error(-this->cb_.result_)};
                }
            }

        private:
            Addr      addr_{};
            socklen_t length_{sizeof(Addr)};
        };
        return Accept{fd_};
    }

public:
    [[nodiscard]]
    static auto bind(const Addr &addr) -> Result<Listener> {
        auto fd = ::socket(addr.family(), SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (fd < 0) [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        if (::bind(fd, addr.sockaddr(), addr.length()) == -1) [[unlikely]] {
            ::close(fd);
            return std::unexpected{make_sys_error(errno)};
        }
        if (::listen(fd, SOMAXCONN) == -1) [[unlikely]] {
            ::close(fd);
            return std::unexpected{make_sys_error(errno)};
        }
        return Listener{fd};
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
        return std::unexpected{make_zedio_error(Error::InvalidAddresses)};
    }
};

} // namespace zedio::socket::detail
