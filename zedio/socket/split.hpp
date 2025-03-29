#pragma once

#include "zedio/io/impl/impl_stream_read.hpp"
#include "zedio/io/impl/impl_stream_write.hpp"
#include "zedio/socket/impl/impl_sockopt.hpp"

namespace zedio::socket::detail {

template <class Addr>
class ReadHalf : public io::detail::ImplStreamRead<ReadHalf<Addr>>,
                 public ImplLocalAddr<ReadHalf<Addr>, Addr>,
                 public ImplPeerAddr<ReadHalf<Addr>, Addr> {
public:
    ReadHalf(Socket &inner)
        : inner{inner} {}

public:
    [[nodiscard]]
    auto handle(this const ReadHalf &self) noexcept {
        return self.inner.handle();
    }

private:
    Socket &inner;
};

template <class Addr>
class WriteHalf : public io::detail::ImplStreamWrite<WriteHalf<Addr>>,
                  public ImplLocalAddr<WriteHalf<Addr>, Addr>,
                  public ImplPeerAddr<WriteHalf<Addr>, Addr> {
public:
    WriteHalf(Socket &inner)
        : inner{inner} {}

public:
    [[nodiscard]]
    auto handle(this const WriteHalf &self) noexcept {
        return self.inner.handle();
    }

private:
    Socket &inner;
};

template <class T>
class OwnedBase {
protected:
    OwnedBase(std::shared_ptr<T> stream)
        : stream{stream} {}

public:
    [[nodiscard]]
    auto handle(this const OwnedBase &self) noexcept {
        return self.innter.handle();
    }

    auto reunite(this OwnedBase &self, OwnedBase &other) -> Result<T> {
        if (std::addressof(self) == std::addressof(other) || self.stream != other.stream) {
            return std::unexpected{make_error(Error::ReuniteFailed)};
        }
        other.stream.reset();
        std::shared_ptr<T> result{nullptr};
        result.swap(stream);
        return std::move(*result);
    }

protected:
    std::shared_ptr<T> stream;
};

template <class T, class Addr>
class OwnedReadHalf : public OwnedBase<T>,
                      public io::detail::ImplStreamRead<OwnedReadHalf<T, Addr>>,
                      public ImplLocalAddr<OwnedReadHalf<T, Addr>, Addr>,
                      public ImplPeerAddr<OwnedReadHalf<T, Addr>, Addr> {

public:
    explicit OwnedReadHalf(std::shared_ptr<T> stream)
        : OwnedBase<T>{stream} {}

    // ~OwnedReadHalf() {
    //     if (this->stream_) {
    //         // TODO register shutdown write
    //     }
    // }
};

template <class T, class Addr>
class OwnedWriteHalf : public OwnedBase<T>,
                       public io::detail::ImplStreamWrite<OwnedWriteHalf<T, Addr>>,
                       public ImplLocalAddr<OwnedWriteHalf<T, Addr>, Addr>,
                       public ImplPeerAddr<OwnedWriteHalf<T, Addr>, Addr> {

public:
    explicit OwnedWriteHalf(std::shared_ptr<T> stream)
        : OwnedBase<T>{stream} {}

    // ~OwnedWriteHalf() {
    //     if (this->stream_) {
    //         // TODO register shutdown read
    //     }
    // }
};

} // namespace zedio::socket::detail
