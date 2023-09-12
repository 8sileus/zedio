#pragma once

#include "async/task.hpp"

// Linux
#include <liburing.h>

namespace zed::async {
namespace detail {
io_uring *t_ring;
} // namespace detail

void sleep_for(const std::chrono::milliseconds &mill);

[[nodiscard]]
auto accept(int fd, sockaddr *addr, socklen_t *addrlen, int flags, bool shared = true)
    -> Task<ssize_t>;

[[nodiscard]]
auto connect(int fd, const sockaddr *addr, socklen_t addrlen, bool shared = true) -> Task<ssize_t>;

[[nodiscard]]
auto read(int fd, void *buf, std::size_t nbytes, uint64_t offset, bool shared = true)
    -> Task<ssize_t>;

[[nodiscard]]
auto readv(int fd, const iovec *iovecs, int nr_vecs, uint64_t offset, bool shared = true)
    -> Task<ssize_t>;

[[nodiscard]]
auto readv2(int fd, const iovec *iovecs, int nr_vecs, uint64_t offset, int flags,
            bool shared = true) -> Task<ssize_t>;

[[nodiscard]]
auto write(int fd, const void *buf, unsigned nbytes, uint64_t offset, bool shared = true)
    -> Task<ssize_t>;

[[nodiscard]]
auto writev(int fd, const iovec *iovecs, unsigned nr_vecs, uint64_t offset, bool shared = true)
    -> Task<ssize_t>;

[[nodiscard]]
auto writev2(int fd, const iovec *iovecs, unsigned nr_vecs, uint64_t offset, int flags,
             bool shared = true) -> Task<ssize_t>;

[[nodiscard]]
auto recv(int sockfd, void *buf, std::size_t len, int flags, bool shared = true) -> Task<ssize_t>;

[[nodiscard]]
auto send(int sockfd, const void *buf, std::size_t len, int flags, bool shared = true)
    -> Task<ssize_t>;

} // namespace zed::async