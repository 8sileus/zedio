#pragma once

#include "async/async_io.hpp"
#include "async/io_awaiter.hpp"
#include "async/processor.hpp"

// Linux
#include <unistd.h>
// C
#include <cassert>

namespace zed::async {

namespace detail {
[[nodiscard]]
auto wait_sqe() -> Task<io_uring_sqe *> {
    while (true) {
        co_await YieldAwaiter();
        auto sqe = io_uring_get_sqe(t_ring);
        if (sqe != nullptr) {
            co_return sqe;
        }
    }
}
} // namespace detail

void sleep_for(const std::chrono::milliseconds &mill) {
    // TODO
}

[[nodiscard]]
auto accept(int fd, sockaddr *addr, socklen_t *addrlen, int flags, bool shared = true)
    -> Task<ssize_t> {
    assert(detail::t_ring != nullptr);
    auto sqe = io_uring_get_sqe(detail::t_ring);
    if (sqe == nullptr) {
        sqe = co_await detail::wait_sqe();
    }
    io_uring_prep_accept(sqe, fd, addr, addrlen, flags);
    co_return co_await IOAwaiter(sqe, shared);
}

[[nodiscard]]
auto connect(int fd, const sockaddr *addr, socklen_t addrlen, bool shared = true) -> Task<ssize_t> {
    assert(detail::t_ring != nullptr);
    auto sqe = io_uring_get_sqe(detail::t_ring);
    if (sqe == nullptr) {
        sqe = co_await detail::wait_sqe();
    }
    io_uring_prep_connect(sqe, fd, addr, addrlen);
    co_return co_await IOAwaiter(sqe, shared);
}

[[nodiscard]]
auto read(int fd, void *buf, std::size_t nbytes, uint64_t offset, bool shared = true)
    -> Task<ssize_t> {
    assert(detail::t_ring != nullptr);
    auto sqe = io_uring_get_sqe(detail::t_ring);
    if (sqe == nullptr) {
        sqe = co_await detail::wait_sqe();
    }
    io_uring_prep_read(sqe, fd, buf, nbytes, offset);
    co_return co_await IOAwaiter(sqe, shared);
}

[[nodiscard]]
auto readv(int fd, const iovec *iovecs, int nr_vecs, uint64_t offset, bool shared = true)
    -> Task<ssize_t> {
    assert(detail::t_ring != nullptr);
    auto sqe = io_uring_get_sqe(detail::t_ring);
    if (sqe == nullptr) {
        sqe = co_await detail::wait_sqe();
    }
    io_uring_prep_readv(sqe, fd, iovecs, nr_vecs, offset);
    co_return co_await IOAwaiter(sqe, shared);
}

[[nodiscard]]
auto readv2(int fd, const iovec *iovecs, int nr_vecs, uint64_t offset, int flags,
            bool shared = true) -> Task<ssize_t> {
    assert(detail::t_ring != nullptr);
    auto sqe = io_uring_get_sqe(detail::t_ring);
    if (sqe == nullptr) {
        sqe = co_await detail::wait_sqe();
    }
    io_uring_prep_readv2(sqe, fd, iovecs, nr_vecs, offset, flags);
    co_return co_await IOAwaiter(sqe, shared);
}

[[nodiscard]]
auto write(int fd, const void *buf, unsigned nbytes, uint64_t offset, bool shared = true)
    -> Task<ssize_t> {
    assert(detail::t_ring != nullptr);
    auto sqe = io_uring_get_sqe(detail::t_ring);
    if (sqe == nullptr) {
        sqe = co_await detail::wait_sqe();
    }
    io_uring_prep_write(sqe, fd, buf, nbytes, offset);
    co_return co_await IOAwaiter(sqe, shared);
}

[[nodiscard]]
auto writev(int fd, const iovec *iovecs, unsigned nr_vecs, uint64_t offset, bool shared = true)
    -> Task<ssize_t> {
    assert(detail::t_ring != nullptr);
    auto sqe = io_uring_get_sqe(detail::t_ring);
    if (sqe == nullptr) {
        sqe = co_await detail::wait_sqe();
    }
    io_uring_prep_writev(sqe, fd, iovecs, nr_vecs, offset);
    co_return co_await IOAwaiter(sqe, shared);
}

[[nodiscard]]
auto writev2(int fd, const iovec *iovecs, unsigned nr_vecs, uint64_t offset, int flags,
             bool shared = true) -> Task<ssize_t> {
    assert(detail::t_ring != nullptr);
    auto sqe = io_uring_get_sqe(detail::t_ring);
    if (sqe == nullptr) {
        sqe = co_await detail::wait_sqe();
    }
    io_uring_prep_writev2(sqe, fd, iovecs, nr_vecs, offset, flags);
    co_return co_await IOAwaiter(sqe, shared);
}

[[nodiscard]]
auto recv(int sockfd, void *buf, std::size_t len, int flags, bool shared = true) -> Task<ssize_t> {
    assert(detail::t_ring != nullptr);
    auto sqe = io_uring_get_sqe(detail::t_ring);
    if (sqe == nullptr) {
        sqe = co_await detail::wait_sqe();
    }
    io_uring_prep_recv(sqe, sockfd, buf, len, flags);
    co_return co_await IOAwaiter(sqe, shared);
}

[[nodiscard]]
auto send(int sockfd, const void *buf, std::size_t len, int flags, bool shared = true)
    -> Task<ssize_t> {
    assert(detail::t_ring != nullptr);
    auto sqe = io_uring_get_sqe(detail::t_ring);
    if (sqe == nullptr) {
        sqe = co_await detail::wait_sqe();
    }
    io_uring_prep_send(sqe, sockfd, buf, len, flags);
    co_return co_await IOAwaiter(sqe, shared);
}

} // namespace zed::async