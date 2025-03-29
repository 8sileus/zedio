#pragma once

#include "zedio/socket/impl/impl_local_addr.hpp"
#include "zedio/socket/socket.hpp"

namespace zedio::socket::detail {

template <class Listener, class Stream, class Addr>
class BaseListener : public ImplLocalAddr<BaseListener<Listener, Stream, Addr>, Addr> {
protected:
    explicit BaseListener(Socket &&inner)
        : inner{std::move(inner)} {}

public:
    [[REMEMBER_CO_AWAIT]]
    auto accept(this const BaseListener &self) noexcept {
        class Accept : public io::detail::Accept {
        public:
            Accept(SocketHandle listen_socket)
                : io::detail::Accept{listen_socket, nullptr, &length} {
                io::detail::Accept::addr = addr.sockaddr();
            }

            auto await_resume() noexcept -> Result<std::pair<Stream, Addr>> {
                auto ret = io::detail::Accept::await_resume();
                if (!ret) [[unlikely]] {
                    return std::unexpected{ret.error()};
                }
                return std::make_pair(Stream{Socket{ret.value()}}, addr);
            }

        private:
            Addr        addr{};
            SocketLength length{sizeof(Addr)};
        };
        return Accept{self.handle()};
    }

    [[nodiscard]]
    auto handle(this const BaseListener &self) noexcept {
        return self.inner.handle();
    }

public:
    [[nodiscard]]
    static auto bind(const Addr &addr) -> Result<Listener> {
        auto ret = Socket::create(addr.family(), SOCK_STREAM, 0);
        if (!ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        auto &inner = ret.value();
        if (auto ret = inner.bind(addr); !ret) [[unlikely]]
        {
            return std::unexpected{ret.error()};
        }
        if (auto ret = inner.listen(); !ret) [[unlikely]]
        {
            return std::unexpected{ret.error()};
        }
        return Listener{std::move(inner)};
    }

    [[nodiscard]]
    static auto bind(const std::span<Addr> &addresses) -> Result<Listener> {
        for (const auto &address : addresses) {
            if (auto ret = bind(address); ret) [[likely]]
            {
                return ret;
            } else {
                LOG_ERROR("Bind {} failed, error: {}", address.to_string(), ret.error().message());
            }
        }
        return std::unexpected{make_error(Error::InvalidAddresses)};
    }

private:
    Socket inner;
};

} // namespace zedio::socket::detail
