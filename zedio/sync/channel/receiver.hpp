#pragma once

// C++
#include <memory>

namespace zedio::sync::detail {

template <typename C>
class Receiver {
    using ChannelPtr = std::shared_ptr<C>;

public:
    Receiver(ChannelPtr counter)
        : channel_{counter} {}

    ~Receiver() {
        close();
    }

    Receiver(const Receiver &other)
        : channel_{other.channel_} {
        channel_->add_receiver();
    }

    auto operator=(const Receiver &other) -> Receiver & {
        channel_->sub_receiver();
        channel_ = other.channel_;
        channel_->add_receiver();
        return *this;
    }

    Receiver(Receiver &&other)
        : channel_{std::move(other.channel_)} {}

    auto operator=(Receiver &&other) -> Receiver & {
        channel_->sub_receiver();
        channel_ = std::move(other->channel_);
        return *this;
    }

    [[REMEMBER_CO_AWAIT]]
    auto recv() {
        return channel_->recv();
    }

    void close() {
        if (channel_) {
            channel_->sub_receiver();
            channel_.reset();
        }
    }

private:
    ChannelPtr channel_;
};

} // namespace zedio::sync::detail