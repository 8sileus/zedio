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
struct Accept : public detail::LazyBaseIOAwaiter {
    Accept(int fd, sockaddr *addr, socklen_t *addrlen, int flags)
        : LazyBaseIOAwaiter(T::tid(), [fd, addr, addrlen, flags](io_uring_sqe *sqe) {
            io_uring_prep_accept(sqe, fd, addr, addrlen, flags);
        }) {}
};

template <HasTid T = Dispatch>
struct Close : public detail::LazyBaseIOAwaiter {
    Close(int fd)
        : LazyBaseIOAwaiter(T::tid(), [fd](io_uring_sqe *sqe) { io_uring_prep_close(sqe, fd); }) {}
};

template <HasTid T = Dispatch>
struct Connect : public detail::LazyBaseIOAwaiter {
    Connect(int fd, const sockaddr *addr, socklen_t addrlen)
        : LazyBaseIOAwaiter(T::tid(), [fd, addr, addrlen](io_uring_sqe *sqe) {
            io_uring_prep_connect(sqe, fd, addr, addrlen);
        }) {}
};

template <HasTid T = Dispatch>
struct Read : public detail::LazyBaseIOAwaiter {
    Read(int fd, void *buf, std::size_t nbytes, uint64_t offset = 0)
        : LazyBaseIOAwaiter(T::tid(), [fd, buf, nbytes, offset](io_uring_sqe *sqe) {
            io_uring_prep_read(sqe, fd, buf, nbytes, offset);
        }) {}
};

template <HasTid T = Dispatch>
struct Readv : public detail::LazyBaseIOAwaiter {
    Readv(int fd, const iovec *iovecs, int nr_vecs, uint64_t offset = 0)
        : LazyBaseIOAwaiter(T::tid(), [fd, iovecs, nr_vecs, offset](io_uring_sqe *sqe) {
            io_uring_prep_readv(sqe, fd, iovecs, nr_vecs, offset);
        }) {}
};

template <HasTid T = Dispatch>
struct Readv2 : public detail::LazyBaseIOAwaiter {
    Readv2(int fd, const iovec *iovecs, int nr_vecs, uint64_t offset, int flags)
        : LazyBaseIOAwaiter(T::tid(), [fd, iovecs, nr_vecs, offset, flags](io_uring_sqe *sqe) {
            io_uring_prep_readv2(sqe, fd, iovecs, nr_vecs, offset, flags);
        }) {}
};

template <HasTid T = Dispatch>
struct Recv : public detail::LazyBaseIOAwaiter {
    Recv(int sockfd, void *buf, std::size_t len, int flags)
        : LazyBaseIOAwaiter(T::tid(), [sockfd, buf, len, flags](io_uring_sqe *sqe) {
            io_uring_prep_recv(sqe, sockfd, buf, len, flags);
        }) {}
};

template <HasTid T = Dispatch>
struct Send : public detail::LazyBaseIOAwaiter {
    Send(int sockfd, const void *buf, std::size_t len, int flags)
        : LazyBaseIOAwaiter(T::tid(), [sockfd, buf, len, flags](io_uring_sqe *sqe) {
            io_uring_prep_send(sqe, sockfd, buf, len, flags);
        }) {}
};

template <HasTid T = Dispatch>
struct Shutdown : public detail::LazyBaseIOAwaiter {
    Shutdown(int fd, int how)
        : LazyBaseIOAwaiter(
            T::tid(), [fd, how](io_uring_sqe *sqe) { io_uring_prep_shutdown(sqe, fd, how); }) {}
};

template <HasTid T = Dispatch>
struct Socket : public detail::LazyBaseIOAwaiter {
    Socket(int domain, int type, int protocol, int flags = 0)
        : LazyBaseIOAwaiter(T::tid(), [domain, type, protocol, flags](io_uring_sqe *sqe) {
            io_uring_prep_socket(sqe, domain, type, protocol, flags);
        }) {}
};

template <HasTid T = Dispatch>
struct Write : public detail::LazyBaseIOAwaiter {
    Write(int fd, const void *buf, unsigned nbytes, uint64_t offset = 0)
        : LazyBaseIOAwaiter(T::tid(), [fd, buf, nbytes, offset](io_uring_sqe *sqe) {
            io_uring_prep_write(sqe, fd, buf, nbytes, offset);
        }) {}
};

template <HasTid T = Dispatch>
struct Writev : public detail::LazyBaseIOAwaiter {
    Writev(int fd, const iovec *iovecs, unsigned nr_vecs, uint64_t offset = 0)
        : LazyBaseIOAwaiter(T::tid(), [fd, iovecs, nr_vecs, offset](io_uring_sqe *sqe) {
            io_uring_prep_writev(sqe, fd, iovecs, nr_vecs, offset);
        }) {}
};

template <HasTid T = Dispatch>
struct Writev2 : public detail::LazyBaseIOAwaiter {
    Writev2(int fd, const iovec *iovecs, unsigned nr_vecs, uint64_t offset, int flags)
        : LazyBaseIOAwaiter(T::tid(), [fd, iovecs, nr_vecs, offset, flags](io_uring_sqe *sqe) {
            io_uring_prep_writev2(sqe, fd, iovecs, nr_vecs, offset, flags);
        }) {}
};

struct Bind : public detail::EagerBaseIOAwaiter {
    Bind(int fd, const sockaddr *addr, socklen_t len)
        : EagerBaseIOAwaiter(::bind(fd, addr, len)) {}
};

struct Listen : public detail::EagerBaseIOAwaiter {
    Listen(int fd, int n)
        : EagerBaseIOAwaiter(::listen(fd, n)) {}
};

} // namespace zed::async
