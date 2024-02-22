#pragma once

#include "zedio/async/io/registrator.hpp"
#include "zedio/common/macros.hpp"

namespace zedio::async::detail {

template <Mode mode = Mode::S>
struct [[REMEMBER_CO_AWAIT]] SocketAwaiter : public IORegistrator<mode, int> {
    SocketAwaiter(int domain, int type, int protocol, unsigned int flags)
        : IORegistrator<mode, int>{io_uring_prep_socket, domain, type, protocol, flags} {}
};

template <Mode mode = Mode::S>
struct [[REMEMBER_CO_AWAIT]] AcceptAwaiter : public IORegistrator<mode, int> {
    AcceptAwaiter(int fd, sockaddr *addr, socklen_t *addrlen, int flags)
        : IORegistrator<mode, int>{io_uring_prep_accept, fd, addr, addrlen, flags} {}
};

template <Mode mode = Mode::S>
struct [[REMEMBER_CO_AWAIT]] ConnectAwaiter : public IORegistrator<mode, void> {
    ConnectAwaiter(int fd, const sockaddr *addr, socklen_t addrlen)
        : IORegistrator<mode, void>{io_uring_prep_connect, fd, addr, addrlen} {}
};

template <Mode mode = Mode::S>
struct [[REMEMBER_CO_AWAIT]] ShutdownAwaiter : public IORegistrator<mode, void> {
    ShutdownAwaiter(int fd, int how)
        : IORegistrator<mode, void>{io_uring_prep_shutdown, fd, how} {}
};

template <Mode mode = Mode::S>
struct [[REMEMBER_CO_AWAIT]] CloseAwaiter : public IORegistrator<mode, void> {
    CloseAwaiter(int fd)
        : IORegistrator<mode, void>{io_uring_prep_close, fd} {}
};

template <Mode mode = Mode::S>
struct [[REMEMBER_CO_AWAIT]] RecvAwaiter : public IORegistrator<mode, std::size_t> {
    RecvAwaiter(int sockfd, void *buf, std::size_t len, int flags)
        : IORegistrator<mode, std::size_t>{io_uring_prep_recv, sockfd, buf, len, flags} {}
};

template <Mode mode>
struct [[REMEMBER_CO_AWAIT]] RecvMsgAwaiter : public IORegistrator<mode, std::size_t> {
    RecvMsgAwaiter(int fd, msghdr *msg, unsigned flags)
        : IORegistrator<mode, std::size_t>{io_uring_prep_recvmsg, fd, msg, flags} {}
};

template <Mode mode = Mode::S>
struct [[REMEMBER_CO_AWAIT]] SendAwaiter : public IORegistrator<mode, std::size_t> {
    SendAwaiter(int sockfd, const void *buf, std::size_t len, int flags)
        : IORegistrator<mode, std::size_t>{io_uring_prep_send, sockfd, buf, len, flags} {}
};

template <Mode mode = Mode::S>
struct [[REMEMBER_CO_AWAIT]] SendMsgAwaiter : public IORegistrator<mode, std::size_t> {
    SendMsgAwaiter(int fd, const msghdr *msg, unsigned flags)
        : IORegistrator<mode, std::size_t>{io_uring_prep_sendmsg, fd, msg, flags} {}
};

template <Mode mode = Mode::S>
struct [[REMEMBER_CO_AWAIT]] SendToAwaiter : public IORegistrator<mode, std::size_t> {
    SendToAwaiter(int             sockfd,
                  const void     *buf,
                  size_t          len,
                  int             flags,
                  const sockaddr *addr,
                  socklen_t       addrlen)
        : IORegistrator<mode, std::size_t>{io_uring_prep_sendto,
                                           sockfd,
                                           buf,
                                           len,
                                           flags,
                                           addr,
                                           addrlen} {}
};

template <Mode mode = Mode::S>
struct [[REMEMBER_CO_AWAIT]] ReadAwaiter : public IORegistrator<mode, std::size_t> {
    ReadAwaiter(int fd, void *buf, std::size_t nbytes, uint64_t offset)
        : IORegistrator<mode, std::size_t>{io_uring_prep_read, fd, buf, nbytes, offset} {}
};

template <Mode mode = Mode::S>
struct [[REMEMBER_CO_AWAIT]] ReadvAwaiter : public IORegistrator<mode, std::size_t> {
    ReadvAwaiter(int fd, iovec *iovecs, int nr_vecs, uint64_t offset)
        : IORegistrator<mode, std::size_t>{io_uring_prep_readv, fd, iovecs, nr_vecs, offset} {}
};

// template <Mode mode>
// struct [[REMEMBER_CO_AWAIT]] Readv2Awaiter : public IORegistrator {
//     Readv2Awaiter(int fd, iovec *iovecs, int nr_vecs, uint64_t offset, int flags) {
//         if (this->sqe_) [[likely]] {

//             io_uring_prep_readv2(this->sqe_, fd, iovecs, nr_vecs, offset, flags);
//         }
//     }
// };

template <Mode mode = Mode::S>
struct [[REMEMBER_CO_AWAIT]] WriteAwaiter : public IORegistrator<mode, std::size_t> {
    WriteAwaiter(int fd, const void *buf, unsigned nbytes, uint64_t offset)
        : IORegistrator<mode, std::size_t>{io_uring_prep_write, fd, buf, nbytes, offset} {}
};

template <Mode mode = Mode::S>
struct [[REMEMBER_CO_AWAIT]] WritevAwaiter : public IORegistrator<mode, std::size_t> {
    WritevAwaiter(int fd, const iovec *iovecs, unsigned nr_vecs, uint64_t offset)
        : IORegistrator<mode, std::size_t>{io_uring_prep_writev, fd, iovecs, nr_vecs, offset} {}
};

// template <Mode mode>
// struct [[REMEMBER_CO_AWAIT]] Writev2Awaiter : public IORegistrator {
//     Writev2Awaiter(int fd, const iovec *iovecs, unsigned nr_vecs, uint64_t offset, int flags)
//     {
//         if (this->sqe_) [[likely]] {
//             io_uring_prep_writev2(this->sqe_, fd, iovecs, nr_vecs, offset, flags);
//         }
//     }
// };

template <Mode mode = Mode::S>
struct [[REMEMBER_CO_AWAIT]] OpenAt2Awaiter : public IORegistrator<mode, int> {
    OpenAt2Awaiter(int dfd, const char *path, struct open_how *how)
        : IORegistrator<mode, int>{io_uring_prep_openat2, dfd, path, how} {}
};

template <Mode mode = Mode::S>
struct [[REMEMBER_CO_AWAIT]] OpenAtAwaiter : public IORegistrator<mode, int> {
    OpenAtAwaiter(int dfd, const char *path, int flags, mode_t permission)
        : IORegistrator<mode, int>{io_uring_prep_openat, dfd, path, flags, permission} {}
};

template <Mode mode = Mode::S>
struct [[REMEMBER_CO_AWAIT]] StatxAwaiter : public IORegistrator<mode, void> {
    StatxAwaiter(int dfd, const char *path, int flags, unsigned mask, struct statx *statxbuf)
        : IORegistrator<mode, void>{io_uring_prep_statx, dfd, path, flags, mask, statxbuf} {}
};

template <Mode mode = Mode::S>
struct [[REMEMBER_CO_AWAIT]] FsyncAwaiter : public IORegistrator<mode, void> {
    FsyncAwaiter(int fd, unsigned fsync_flags)
        : IORegistrator<mode, void>{io_uring_prep_fsync, fd, fsync_flags} {}
};

} // namespace zedio::async::detail
