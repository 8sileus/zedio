#pragma once

#include "async.hpp"
#include "common/error.hpp"
#include "log.hpp"
#include "net/address.hpp"
#include "net/detail/stream_buffer.hpp"
#include "net/tcp_listener.hpp"
#include "net/tcp_stream.hpp"

// C++
#include <functional>
#include <memory>
#include <optional>

namespace zed::net {

class Server : util::Noncopyable {
public:
    Server(std::size_t worker_num = std::thread::hardware_concurrency())
        : dispatcher_{worker_num} {}

    void start() { this->accept_processor_.start(); }

    [[nodiscard]]
    auto bind(const Address &address) -> std::error_code {
        TcpListener listener;
        if (auto err = listener.bind(address); err) {
            return err;
        }
        this->accept_processor_.post(accept_stream(std::move(listener)));
        return std::error_code{};
    }

private:
    [[nodiscard]]
    auto accept_stream(TcpListener &&listener) -> async::Task<void> {
        std::optional<TcpStream> stream;
        while (true) {
            if (stream = co_await listener.accept(); stream) {
                LOG_DEBUG("Accept a connection from {}",
                          stream.value().get_peer_address().to_string());
                this->accept_processor_.post(handle_stream(std::move(stream.value())));
            }
        }
    }

    auto handle_stream(TcpStream &&stream) -> async::Task<void> {
        detail::StreamBuffer input_buffer;
        detail::StreamBuffer output_buffer;
        int                  n;
        char                 extra_buf[config::EXTRA_BUFFER_SUZE];
        struct iovec         iovecs[2];
        constexpr int        nr_vecs = 2;
        while (true) {
            iovecs[0].iov_base = input_buffer.begin_write();
            iovecs[0].iov_len = input_buffer.writeable_bytes();
            iovecs[1].iov_base = extra_buf;
            iovecs[1].iov_len = sizeof(extra_buf);
            n = co_await stream.readv(iovecs, nr_vecs);
            if (n <= 0) [[unlikely]] {
                if (n != 0) {
                    LOG_ERROR("Socket read failed on fd {}, error: {}", stream.get_fd(),
                              strerror(-n));
                }
                break;
            } else {
                input_buffer.increase_write_index(n);
            }

            n = co_await stream.write(output_buffer.begin_read(), output_buffer.readable_bytes());
            if (n <= 0) [[unlikely]] {
                LOG_ERROR("Socket write failed on fd {}, error: {}", stream.get_fd(), strerror(-n));
            } else {
                input_buffer.increase_read_index(n);
            }
        }
    }

private:
    std::function<async::Task<void>(TcpStream &)> cb_{};
    async::detail::Processor                      accept_processor_{};
    async::detail::Dispatcher                     dispatcher_;
};

} // namespace zed::net
