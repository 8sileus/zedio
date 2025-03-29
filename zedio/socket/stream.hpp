#pragma once

#include "zedio/socket/impl/impl_local_addr.hpp"
#include "zedio/socket/impl/impl_peer_addr.hpp"
#include "zedio/socket/socket.hpp"
#include "zedio/socket/split.hpp"
// C++
#include <utility>

namespace zedio::socket::detail {

template <class Stream, class Addr>
class BaseStream : public io::detail::ImplStreamRead<BaseStream<Stream, Addr>>,
                   public io::detail::ImplStreamWrite<BaseStream<Stream, Addr>>,
                   public ImplLocalAddr<BaseStream<Stream, Addr>, Addr>,
                   public ImplPeerAddr<BaseStream<Stream, Addr>, Addr> {
public:
    using Reader = ReadHalf<Addr>;
    using Writer = WriteHalf<Addr>;
    using OwnedReader = OwnedReadHalf<Stream, Addr>;
    using OwnedWriter = OwnedWriteHalf<Stream, Addr>;

protected:
    explicit BaseStream(Socket &&inner)
        : inner{std::move(inner)} {}

public:
    [[REMEMBER_CO_AWAIT]]
    auto shutdown(this BaseStream &self, io::ShutdownBehavior how) noexcept {
        return self.inner.shutdown(how);
    }

    [[REMEMBER_CO_AWAIT]]
    auto close(this BaseStream &self) noexcept {
        return self.inner.close();
    }

    [[nodiscard]]
    auto handle(this const BaseStream &self) noexcept {
        return self.inner.handle();
    }

    [[nodiscard]]
    auto split(this const BaseStream &self) noexcept -> std::pair<Reader, Writer> {
        return std::make_pair(Reader{self.inner}, Writer{self.inner});
    }

    [[nodiscard]]
    auto into_split(this BaseStream &self) noexcept -> std::pair<OwnedReader, OwnedWriter> {
        auto stream = std::make_shared<Stream>(std::move(self.inner));
        return std::make_pair(OwnedReader{stream}, OwnedWriter{stream});
    }

public:
    [[REMEMBER_CO_AWAIT]]
    static auto connect(const Addr &addr) noexcept -> async::Task<Result<Stream>> {

        auto sock = Socket::create(addr.family(), SOCK_STREAM, 0);
        if (!sock) [[unlikely]] {
            co_return std::unexpected{sock.error()};
        }

        auto res
            = co_await io::detail::Connect{sock.value().handle(), addr.sockaddr(), addr.length()};
        if (!res) [[unlikely]] {
            co_return std::unexpected{res.error()};
        }

        co_return Stream{std::move(sock.value())};
    }

private:
    Socket inner;
};

} // namespace zedio::socket::detail
