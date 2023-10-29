// #pragma once

// #include "async.hpp"
// #include "common/macros.hpp"
// #include "log.hpp"
// #include "net/address.hpp"

// namespace zed::net {

// class TcpServer {
// public:
//     TcpServer(const Address &local_address,
//               std::size_t    worker_num = std::thread::hardware_concurrency())
//         : fd_(::socket(local_address.get_family(), SOCK_STREAM, 0))
//         , local_address_(local_address)
//         , scheduler_(worker_num) {
//         if (fd_ < 0) {
//             throw std::runtime_error(FORMAT_FUNC_ERR_MESSAGE(socket, errno));
//         }
//         if (::bind(fd_, local_address_.get_sockaddr(), local_address_.get_length()) != 0) {
//             throw std::runtime_error(FORMAT_FUNC_ERR_MESSAGE(bind, errno));
//         }
//     }

//     ~TcpServer() {
//         scheduler_.stop();
//         ::close(fd_);
//     }

//     void start() {
//         if (::listen(fd_, listen_queue_length_) != 0) {
//             throw std::runtime_error(FORMAT_FUNC_ERR_MESSAGE(listen, errno));
//         }
//         scheduler_.submit(Reactor());
//         scheduler_.start();
//     }

//     auto get_listen_queue_length() const -> int { return listen_queue_length_; }

//     void set_listen_queue_length(int length) { listen_queue_length_ = length; }

//     // void set_callback(std::function<void>());
// private:
//     auto Reactor() -> async::Task<void> {
//         int peer_fd;
//         Address peer_address;
//         while ((peer_fd = co_await async::Accept(fd_, peer_address)) > 0) {
//             log::zed_logger.debug("{} accept a connection from {}", local_address_.get_ip(),
//                                   peer_address.get_ip());
//             scheduler_.submit(Handler(peer_fd));
//         }
//     }

//     auto Handler(int peer_fd) -> async::Task<void> {
//         std::vector<char> buf(1024);
//         int               ret;
//         while (true) {
//             ret = co_await async::Read(peer_fd, buf.data(), sizeof(buf));
//             if (ret <= 0) {
//                 if (ret < 0) {
//                     log::zed_logger.error(FORMAT_FUNC_ERR_MESSAGE(read, -ret));
//                 }
//                 break;
//             }
//         }
//         ::close(peer_fd);
//     }

// private:
//     int              fd_;
//     int              listen_queue_length_{SOMAXCONN};
//     async::Scheduler scheduler_;
//     Address          local_address_;
// };

// } // namespace zed::net
