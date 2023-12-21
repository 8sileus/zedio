#pragma once

#include "async/io_awaiter.hpp"

namespace zed::async {

using detail::AL;

template <AL level = AL::shared>
struct Accept : public detail::BaseIOAwaiter {
    Accept(int fd, sockaddr *addr, socklen_t *addrlen)
        : BaseIOAwaiter{level} {
        res_ = accept4(fd, addr, addrlen, SOCK_NONBLOCK);
        if (res_ != -1) {
            ready_ = true;
        } else if (errno != EWOULDBLOCK) {
            res_ = errno;
            ready_ = true;
        } else {
            cb_ = [fd, addr, addrlen](io_uring_sqe *sqe) {
                io_uring_prep_accept(sqe, fd, addr, addrlen, SOCK_NONBLOCK);
            };
        }
    }
};

#define X(f1, f2, ...)                                                    \
    res_ = f1(__VA_ARGS__);                                               \
    if (res_ != -1) {                                                     \
        ready_ = true;                                                    \
    } else if (errno != EWOULDBLOCK) {                                    \
        res_ = errno;                                                     \
        ready_ = true;                                                    \
    } else {                                                              \
        cb_ = [__VA_ARGS__](io_uring_sqe *sqe) { f2(sqe, __VA_ARGS__); }; \
    }

template <AL level = AL::shared>
struct Connect : public detail::BaseIOAwaiter {
    Connect(int fd, const sockaddr *addr, socklen_t addrlen)
        : BaseIOAwaiter{level} {
        X(connect, io_uring_prep_connect, fd, addr, addrlen)
    }
};

template <AL level = AL::shared>
struct Shutdown : public detail::BaseIOAwaiter {
    Shutdown(int fd, int how)
        : BaseIOAwaiter{level} {
        X(shutdown, io_uring_prep_shutdown, fd, how)
    }
};

template <AL level = AL::shared>
struct Close : public detail::BaseIOAwaiter {
    Close(int fd)
        : BaseIOAwaiter{level} {
        X(close, io_uring_prep_close, fd)
    }
};


template <AL level = AL::shared>
struct Recv : public detail::BaseIOAwaiter {
    Recv(int sockfd, void *buf, std::size_t len, int flags)
        : BaseIOAwaiter{level} {
        X(recv, io_uring_prep_recv, sockfd, buf, len, flags)
    }
};

template <AL level = AL::shared>
struct RecvMsg : public detail::BaseIOAwaiter {
    RecvMsg(int fd, msghdr *msg, unsigned flags)
        : BaseIOAwaiter{level} {
        X(recvmsg, io_uring_prep_recvmsg, fd, msg, flags)
    }
};

template <AL level = AL::shared>
struct Send : public detail::BaseIOAwaiter {
    Send(int sockfd, const void *buf, std::size_t len, int flags)
        : BaseIOAwaiter{level} {
        X(send, io_uring_prep_send, sockfd, buf, len, flags)
    }
};

template <AL level = AL::shared>
struct SendMsg : public detail::BaseIOAwaiter {
    SendMsg(int fd, const msghdr *msg, unsigned flags)
        : BaseIOAwaiter{level} {
        X(sendmsg, io_uring_prep_sendmsg, fd, msg, flags)
    }
};

template <AL level = AL::shared>
struct SendTo : public detail::BaseIOAwaiter {
    SendTo(int sockfd, const void *buf, size_t len, int flags, const sockaddr *addr,
           socklen_t addrlen)
        : BaseIOAwaiter{level} {
        X(sendto, io_uring_prep_sendto, sockfd, buf, len, flags, addr, addrlen)
    }
};

#undef X

#define X(f1, f2, ...)                                                        \
    res_ = f1(__VA_ARGS__);                                                   \
    if (res_ != -1) {                                                         \
        ready_ = true;                                                        \
    } else if (errno != EWOULDBLOCK) {                                        \
        res_ = errno;                                                         \
        ready_ = true;                                                        \
    } else {                                                                  \
        cb_ = [__VA_ARGS__](io_uring_sqe *sqe) { f2(sqe, __VA_ARGS__, -1); }; \
    }

template <AL level = AL::shared>
struct Read : public detail::BaseIOAwaiter {
    Read(int fd, void *buf, std::size_t nbytes)
        : BaseIOAwaiter{level} {
        X(read, io_uring_prep_read, fd, buf, nbytes)
    }
};

template <AL level = AL::shared>
struct Readv : public detail::BaseIOAwaiter {
    Readv(int fd, const iovec *iovecs, int nr_vecs)
        : BaseIOAwaiter{level} {
        X(readv, io_uring_prep_readv, fd, iovecs, nr_vecs)
    }
};



template <AL level = AL::shared>
struct Write : public detail::BaseIOAwaiter {
    Write(int fd, const void *buf, unsigned nbytes)
        : BaseIOAwaiter{level} {
        X(write, io_uring_prep_write, fd, buf, nbytes)
    }
};

template <AL level = AL::shared>
struct Writev : public detail::BaseIOAwaiter {
    Writev(int fd, const iovec *iovecs, unsigned nr_vecs)
        : BaseIOAwaiter{level} {
        X(writev, io_uring_prep_writev, fd, iovecs, nr_vecs)
    }
};

#undef X


// template <AL level = AL::shared>
// struct Readv2 : public detail::BaseIOAwaiter {
//     Readv2(int fd, const iovec *iovecs, int nr_vecs, uint64_t offset, int flags )
//         : BaseIOAwaiter{level} {
//         auto cb_ = []
//     }
// };

// template <AL level = AL::shared>
// struct Writev2 : public detail::BaseIOAwaiter {
//     Writev2(int fd, const iovec *iovecs, unsigned nr_vecs, uint64_t offset, int flags)
//         : BaseIOAwaiter{level} {
//         X(readv, io_uring_prep_readv, fd, iovecs, nr_vecs, offset)
//     }
// };

template <AL level = AL::shared>
struct Socket : public detail::BaseIOAwaiter {
    Socket(int domain, int type, int protocol)
        : BaseIOAwaiter{level} {
        type |= SOCK_NONBLOCK;
        res_ = socket(domain, type, protocol);
        if (res_ >= 0) {
            ready_ = true;
        } else {
            cb_ = [domain, type, protocol](io_uring_sqe *sqe) {
                // FUTURE:  The flags argument are currently unused 
                io_uring_prep_socket(sqe, domain, type, protocol, 0);
            };
        }
    }
};

} // namespace zed::async
