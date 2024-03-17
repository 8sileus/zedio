#pragma once

#include "zedio/io/io.hpp"
#include "zedio/socket/split.hpp"
// C++
#include <utility>

namespace zedio::socket::detail {

template <class Stream, class Addr>
class BaseStream : public io::detail::Fd,
                   public ImplStreamRead<BaseStream<Stream, Addr>>,
                   public ImplStreamWrite<BaseStream<Stream, Addr>>,
                   public ImplLocalAddr<BaseStream<Stream, Addr>, Addr>,
                   public ImplPeerAddr<BaseStream<Stream, Addr>, Addr> {
public:
    using Reader = ReadHalf<Addr>;
    using Writer = WriteHalf<Addr>;
    using OwnedReader = OwnedReadHalf<Stream, Addr>;
    using OwnedWriter = OwnedWriteHalf<Stream, Addr>;

protected:
    explicit BaseStream(const int fd)
        : Fd{fd} {}

public:
    [[REMEMBER_CO_AWAIT]]
    auto shutdown(io::ShutdownBehavior how) noexcept {
        return io::detail::Shutdown{fd_, how};
    }

public:
    [[REMEMBER_CO_AWAIT]]
    static auto connect(const Addr &addr) {
        class Connect : public io::detail::IORegistrator<Connect> {
        private:
            using Super = io::detail::IORegistrator<Connect>;

        public:
            Connect(const Addr &addr)
                : Super{io_uring_prep_connect, -1, nullptr, sizeof(Addr)}
                , addr_{addr} {}

            auto await_suspend(std::coroutine_handle<> handle) -> bool {
                fd_ = ::socket(addr_.family(), SOCK_STREAM | SOCK_NONBLOCK, 0);
                if (fd_ < 0) [[unlikely]] {
                    this->cb_.result_ = errno;
                    io_uring_prep_nop(this->sqe_);
                    io_uring_sqe_set_data(this->sqe_, nullptr);
                    return false;
                }
                this->sqe_->fd = fd_;
                this->sqe_->addr = (unsigned long)addr_.sockaddr();
                Super::await_suspend(handle);
                return true;
            }

            auto await_resume() noexcept -> Result<Stream> {
                if (this->cb_.result_ >= 0) [[likely]] {
                    return Stream{fd_};
                } else {
                    if (fd_ >= 0) {
                        ::close(fd_);
                    }
                    return std::unexpected{make_sys_error(-this->cb_.result_)};
                }
            }

        private:
            int  fd_;
            Addr addr_;
        };
        return Connect{addr};
    }

    [[nodiscard]]
    auto split() const noexcept -> std::pair<Reader, Writer> {
        return std::make_pair(Reader{fd_}, Writer{fd_});
    }

    [[nodiscard]]
    auto into_split() noexcept -> std::pair<Reader, Writer> {
        auto stream = std::make_shared<Stream>(this->take_fd());
        return std::make_pair(OwnedReader{stream}, OwnedWriter{stream});
    }
};

} // namespace zedio::socket::detail
