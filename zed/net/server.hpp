#pragma once

#include "async.hpp"
#include "common/error.hpp"
#include "log.hpp"
#include "net/detail/stream_buffer.hpp"
#include "net/socket_addr.hpp"
#include "net/tcp_listener.hpp"
#include "net/tcp_stream.hpp"

// C++
#include <expected>
#include <functional>
#include <memory>
#include <optional>

namespace zed::net {

class Server : util::Noncopyable {
public:
    Server(TcpListener &&listener, std::size_t worker_num)
        : dispatcher_{std::make_unique<async::detail::Dispatcher>(worker_num)}
        , accept_processor_{std::make_unique<async::detail::Processor>()} {
        this->accept_processor_->post(accept_stream(std::move(listener)));
    }

    Server(Server &&other)
        : cb_{std::move(other.cb_)}
        , accept_processor_{std::move(other.accept_processor_)}
        , dispatcher_{std::move(other.dispatcher_)} {};

    ~Server() = default;

    void start() { this->accept_processor_->start(); }

private:
    [[nodiscard]]
    auto accept_stream(TcpListener &&listener) -> async::Task<void> {
        while (true) {
            auto result = co_await listener.accept();
            if (result) [[likely]] {
                LOG_DEBUG("Accept a connection from {}",
                          result.value().peer_address().has_value()
                              ? result.value().peer_address().value().to_string()
                              : "unknown");
                this->accept_processor_->post(this->handle_stream(std::move(result.value())));
            } else {
                LOG_DEBUG("Accept failed error: {}", result.error().message());
            }
        }
    }

    auto handle_stream(TcpStream &&stream) -> async::Task<void> {
        std::expected<int, std::error_code> result;
        detail::StreamBuffer                input_buffer;
        detail::StreamBuffer                output_buffer;
        char                                extra_buf[config::EXTRA_BUFFER_SUZE];
        struct iovec                        iovecs[2];
        constexpr int                       nr_vecs = 2;
        while (true) {
            iovecs[0].iov_base = input_buffer.write_begin();
            iovecs[0].iov_len = input_buffer.writeable_bytes();
            iovecs[1].iov_base = extra_buf;
            iovecs[1].iov_len = sizeof(extra_buf);
            if (result = co_await stream.readv(iovecs, nr_vecs); result) [[likely]] {
                input_buffer.increase_write_index(result.value());
            } else {
                LOG_ERROR("read failed on fd {}, error: {}", stream.get_fd(),
                          result.error().message());
                break;
            }

            this->cb_(input_buffer, output_buffer);

            if (result
                = co_await stream.write(output_buffer.read_begin(), output_buffer.readable_bytes());
                result) [[likely]] {
                input_buffer.increase_read_index(result.value());
            } else {
                LOG_ERROR("write failed on fd {}, error: {}", stream.get_fd(),
                          result.error().message());
                break;
            }
        }
    }

public:
    [[nodiscard]]
    static auto bind(const SocketAddr &address,
                     std::size_t    worker_num = std::thread::hardware_concurrency())
        -> std::expected<Server, std::error_code> {
        if (auto result = TcpListener::bind(address); result) {
            return Server{std::move(result.value()), worker_num};
        } else {
            return std::unexpected{result.error()};
        }
    }

    [[nodiscard]]
    static auto bind(const std::vector<SocketAddr> addresses,
                     std::size_t                   worker_num = std::thread::hardware_concurrency())
        -> std::expected<Server, std::error_code> {
        if (auto result = TcpListener::bind(addresses); result) {
            return Server{std::move(result.value()), worker_num};
        } else {
            return std::unexpected{result.error()};
        }
    }

private:
    std::function<async::Task<void>(detail::StreamBuffer &, detail::StreamBuffer &)> cb_{nullptr};
    std::unique_ptr<async::detail::Processor>  accept_processor_;
    std::unique_ptr<async::detail::Dispatcher> dispatcher_;
};

} // namespace zed::net
