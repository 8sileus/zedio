#pragma once

// C++
#include <memory>

namespace zedio::sync::detail {

template <typename C>
class Sender {
    using ChannelPtr = std::shared_ptr<C>;

public:
    Sender(ChannelPtr counter)
        : channel_{counter} {}

    ~Sender() {
        close();
    }

    Sender(const Sender &other)
        : channel_{other.channel_} {
        channel_->add_sender();
    }

    auto operator=(const Sender &other) -> Sender & {
        channel_->sub_sender();
        channel_ = other.channel_;
        channel_->add_sender();
        return *this;
    }

    Sender(Sender &&other)
        : channel_{std::move(other.channel_)} {}

    auto operator=(Sender &&other) -> Sender & {
        channel_->sub_sender();
        channel_ = std::move(other->channel_);
        return *this;
    }

    [[REMEMBER_CO_AWAIT]]
    auto send(C::ValueType value) {
        return channel_->send(std::move(value));
    }

    void close() {
        if (channel_) {
            channel_->sub_sender();
            channel_.reset();
        }
    }

private:
    ChannelPtr channel_;
};

} // namespace zedio::sync::detail
