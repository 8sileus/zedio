#pragma once

#include "zedio/socket/impl/impl_sockopt.hpp"
#include "zedio/socket/impl/impl_stream_read.hpp"
#include "zedio/socket/impl/impl_stream_write.hpp"

namespace zedio::socket::detail {

template <class Addr>
class ReadHalf : public ImplStreamRead<ReadHalf<Addr>>,
                 public ImplLocalAddr<ReadHalf<Addr>, Addr>,
                 public ImplPeerAddr<ReadHalf<Addr>, Addr> {
public:
    ReadHalf(const int &fd)
        : fd_{fd} {}

public:
    int fd() const noexcept {
        return fd_;
    }

private:
    const int &fd_;
};

template <class Addr>
class WriteHalf : public ImplStreamWrite<WriteHalf<Addr>>,
                  public ImplLocalAddr<WriteHalf<Addr>, Addr>,
                  public ImplPeerAddr<WriteHalf<Addr>, Addr> {
public:
    WriteHalf(const int &fd)
        : fd_{fd} {}

public:
    int fd() const noexcept {
        return fd_;
    }

private:
    const int &fd_;
};

template <class T, class Addr>
class OwnedReadHalf : public ImplStreamRead<OwnedReadHalf<T, Addr>>,
                      public ImplLocalAddr<OwnedReadHalf<T, Addr>, Addr>,
                      public ImplPeerAddr<OwnedReadHalf<T, Addr>, Addr> {
public:
    OwnedReadHalf(std::shared_ptr<T> stream)
        : stream_{std::move(stream)} {}

public:
    int fd() const noexcept {
        return stream_.fd();
    }

private:
    std::shared_ptr<T> stream_;
};

template <class T, class Addr>
class OwnedWriteHalf : public ImplStreamWrite<OwnedWriteHalf<T, Addr>>,
                       public ImplLocalAddr<OwnedWriteHalf<T, Addr>, Addr>,
                       public ImplPeerAddr<OwnedWriteHalf<T, Addr>, Addr> {
public:
    OwnedWriteHalf(std::shared_ptr<T> stream)
        : stream_{std::move(stream)} {}

public:
    int fd() const noexcept {
        return stream_.fd();
    }

private:
    std::shared_ptr<T> stream_;
};

} // namespace zedio::socket::detail
