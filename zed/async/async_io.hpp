#pragma once

#include "async/detail/io_awaiter.hpp"

namespace zed::async {

struct Accept : public detail::LazyBaseIOAwaiter {
    Accept(int fd, sockaddr *addr, socklen_t *addrlen, int flags = 0)
        : LazyBaseIOAwaiter([fd, addr, addrlen, flags](io_uring_sqe *sqe) {
            io_uring_prep_accept(sqe, fd, addr, addrlen, flags);
        }) {}
};

struct Close : public detail::LazyBaseIOAwaiter {
    Close(int fd)
        : LazyBaseIOAwaiter([fd](io_uring_sqe *sqe) { io_uring_prep_close(sqe, fd); }) {}
};

struct Connect : public detail::LazyBaseIOAwaiter {
    Connect(int fd, const sockaddr *addr, socklen_t addrlen)
        : LazyBaseIOAwaiter([fd, addr, addrlen](io_uring_sqe *sqe) {
            io_uring_prep_connect(sqe, fd, addr, addrlen);
        }) {}
};

struct Read : public detail::LazyBaseIOAwaiter {
    Read(int fd, void *buf, std::size_t nbytes, uint64_t offset = -1)
        : LazyBaseIOAwaiter([fd, buf, nbytes, offset](io_uring_sqe *sqe) {
            io_uring_prep_read(sqe, fd, buf, nbytes, offset);
        }) {}
};

struct Readv : public detail::LazyBaseIOAwaiter {
    Readv(int fd, const iovec *iovecs, int nr_vecs, uint64_t offset = -1)
        : LazyBaseIOAwaiter([fd, iovecs, nr_vecs, offset](io_uring_sqe *sqe) {
            io_uring_prep_readv(sqe, fd, iovecs, nr_vecs, offset);
        }) {}
};

struct Readv2 : public detail::LazyBaseIOAwaiter {
    Readv2(int fd, const iovec *iovecs, int nr_vecs, uint64_t offset = -1, int flags = 0)
        : LazyBaseIOAwaiter([fd, iovecs, nr_vecs, offset, flags](io_uring_sqe *sqe) {
            io_uring_prep_readv2(sqe, fd, iovecs, nr_vecs, offset, flags);
        }) {}
};

struct Recv : public detail::LazyBaseIOAwaiter {
    Recv(int sockfd, void *buf, std::size_t len, int flags)
        : LazyBaseIOAwaiter([sockfd, buf, len, flags](io_uring_sqe *sqe) {
            io_uring_prep_recv(sqe, sockfd, buf, len, flags);
        }) {}
};

struct RecvMsg : public detail::LazyBaseIOAwaiter {
    RecvMsg(int fd, msghdr *msg, unsigned flags)
        : LazyBaseIOAwaiter(
            [fd, msg, flags](io_uring_sqe *sqe) { io_uring_prep_recvmsg(sqe, fd, msg, flags); }) {}
};

struct Send : public detail::LazyBaseIOAwaiter {
    Send(int sockfd, const void *buf, std::size_t len, int flags)
        : LazyBaseIOAwaiter([sockfd, buf, len, flags](io_uring_sqe *sqe) {
            io_uring_prep_send(sqe, sockfd, buf, len, flags);
        }) {}
};

struct SendTo : public detail::LazyBaseIOAwaiter {
    SendTo(int sockfd, const void *buf, size_t len, int flags, const sockaddr *addr,
           socklen_t addrlen)
        : LazyBaseIOAwaiter([sockfd, buf, len, flags, addr, addrlen](io_uring_sqe *sqe) {
            io_uring_prep_sendto(sqe, sockfd, buf, len, flags, addr, addrlen);
        }) {}
};

struct SendMsg : public detail::LazyBaseIOAwaiter {
    SendMsg(int fd, const msghdr *msg, unsigned flags)
        : LazyBaseIOAwaiter(
            [fd, msg, flags](io_uring_sqe *sqe) { io_uring_prep_sendmsg(sqe, fd, msg, flags); }) {}
};

struct Shutdown : public detail::LazyBaseIOAwaiter {
    Shutdown(int fd, int how)
        : LazyBaseIOAwaiter(
            [fd, how](io_uring_sqe *sqe) { io_uring_prep_shutdown(sqe, fd, how); }) {}
};

struct Socket : public detail::LazyBaseIOAwaiter {
    Socket(int domain, int type, int protocol, int flags = 0)
        : LazyBaseIOAwaiter([domain, type, protocol, flags](io_uring_sqe *sqe) {
            io_uring_prep_socket(sqe, domain, type, protocol, flags);
        }) {}
};

struct Write : public detail::LazyBaseIOAwaiter {
    Write(int fd, const void *buf, unsigned nbytes, uint64_t offset = -1)
        : LazyBaseIOAwaiter([fd, buf, nbytes, offset](io_uring_sqe *sqe) {
            io_uring_prep_write(sqe, fd, buf, nbytes, offset);
        }) {}
};

struct Writev : public detail::LazyBaseIOAwaiter {
    Writev(int fd, const iovec *iovecs, unsigned nr_vecs, uint64_t offset = -1)
        : LazyBaseIOAwaiter([fd, iovecs, nr_vecs, offset](io_uring_sqe *sqe) {
            io_uring_prep_writev(sqe, fd, iovecs, nr_vecs, offset);
        }) {}
};

struct Writev2 : public detail::LazyBaseIOAwaiter {
    Writev2(int fd, const iovec *iovecs, unsigned nr_vecs, uint64_t offset = -1, int flags = 0)
        : LazyBaseIOAwaiter([fd, iovecs, nr_vecs, offset, flags](io_uring_sqe *sqe) {
            io_uring_prep_writev2(sqe, fd, iovecs, nr_vecs, offset, flags);
        }) {}
};

} // namespace zed::async
