#pragma once

// C++
#include <memory>

namespace zedio::sync::detail {

template <typename C>
class Sender {
    using CounterPtr = std::shared_ptr<C>;

public:
    Sender(CounterPtr counter)
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
    CounterPtr channel_;
};

template <typename C>
class Receiver {
    using CounterPtr = std::shared_ptr<C>;

public:
    Receiver(CounterPtr counter)
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
    CounterPtr channel_;
};

} // namespace zedio::sync::detail
