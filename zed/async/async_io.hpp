#pragma once

#include "async/detail/io_awaiter.hpp"
#include "util/thread.hpp"

namespace zed::async {

struct Dispatch {
    static constexpr auto tid() -> pid_t { return 0; }
};

struct Exclusive {
    static auto tid() -> pid_t { return this_thread::get_tid(); }
};

template <typename T>
concept HasTid = requires(T) { T::tid(); };

template <HasTid T = Dispatch>
struct Accept : public detail::BaseIOAwaiter {
    Accept(int fd, sockaddr *addr, socklen_t *addrlen, int flags)
        : BaseIOAwaiter(T::tid(), [fd, addr, addrlen, flags](io_uring_sqe *sqe) {
            io_uring_prep_accept(sqe, fd, addr, addrlen, flags);
        }) {}
};

template <HasTid T = Dispatch>
struct Connect : public detail::BaseIOAwaiter {
    Connect(int fd, const sockaddr *addr, socklen_t addrlen)
        : BaseIOAwaiter(T::tid(), [fd, addr, addrlen](io_uring_sqe *sqe) {
            io_uring_prep_connect(sqe, fd, addr, addrlen);
        }) {}
};

template <HasTid T = Dispatch>
struct Read : public detail::BaseIOAwaiter {
    Read(int fd, void *buf, std::size_t nbytes, uint64_t offset)
        : BaseIOAwaiter(T::tid(), [fd, buf, nbytes, offset](io_uring_sqe *sqe) {
            io_uring_prep_read(sqe, fd, buf, nbytes, offset);
        }) {}
};

template <HasTid T = Dispatch>
struct Readv : public detail::BaseIOAwaiter {
    Readv(int fd, const iovec *iovecs, int nr_vecs, uint64_t offset)
        : BaseIOAwaiter(T::tid(), [fd, iovecs, nr_vecs, offset](io_uring_sqe *sqe) {
            io_uring_prep_readv(sqe, fd, iovecs, nr_vecs, offset);
        }) {}
};

template <HasTid T = Dispatch>
struct Readv2 : public detail::BaseIOAwaiter {
    Readv2(int fd, const iovec *iovecs, int nr_vecs, uint64_t offset, int flags)
        : BaseIOAwaiter(T::tid(), [fd, iovecs, nr_vecs, offset, flags](io_uring_sqe *sqe) {
            io_uring_prep_readv2(sqe, fd, iovecs, nr_vecs, offset, flags);
        }) {}
};

template <HasTid T = Dispatch>
struct Write : public detail::BaseIOAwaiter {
    Write(int fd, const void *buf, unsigned nbytes, uint64_t offset)
        : BaseIOAwaiter(T::tid(), [fd, buf, nbytes, offset](io_uring_sqe *sqe) {
            io_uring_prep_write(sqe, fd, buf, nbytes, offset);
        }) {}
};

template <HasTid T = Dispatch>
struct Writev : public detail::BaseIOAwaiter {
    Writev(int fd, const iovec *iovecs, unsigned nr_vecs, uint64_t offset)
        : BaseIOAwaiter(T::tid(), [fd, iovecs, nr_vecs, offset](io_uring_sqe *sqe) {
            io_uring_prep_writev(sqe, fd, iovecs, nr_vecs, offset);
        }) {}
};

template <HasTid T = Dispatch>
struct Writev2 : public detail::BaseIOAwaiter {
    Writev2(int fd, const iovec *iovecs, unsigned nr_vecs, uint64_t offset, int flags)
        : BaseIOAwaiter(T::tid(), [fd, iovecs, nr_vecs, offset, flags](io_uring_sqe *sqe) {
            io_uring_prep_writev2(sqe, fd, iovecs, nr_vecs, offset, flags);
        }) {}
};

template <HasTid T = Dispatch>
struct Recv : public detail::BaseIOAwaiter {
    Recv(int sockfd, void *buf, std::size_t len, int flags)
        : BaseIOAwaiter(T::tid(), [sockfd, buf, len, flags](io_uring_sqe *sqe) {
            io_uring_prep_recv(sqe, sockfd, buf, len, flags);
        }) {}
};

template <HasTid T = Dispatch>
struct Send : public detail::BaseIOAwaiter {
    Send(int sockfd, const void *buf, std::size_t len, int flags)
        : BaseIOAwaiter(T::tid(), [sockfd, buf, len, flags](io_uring_sqe *sqe) {
            io_uring_prep_send(sqe, sockfd, buf, len, flags);
        }) {}
};

} // namespace zed::async
