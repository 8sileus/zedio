#pragma once

#include "zedio/sync/channel/channel.hpp"
#include "zedio/sync/channel/receiver.hpp"
#include "zedio/sync/channel/sender.hpp"

namespace zedio::sync {

template <typename T>
class Channel {
    using ChannelImpl = detail::Channel<T>;

public:
    using Sender = detail::Sender<ChannelImpl>;
    using Receiver = detail::Receiver<ChannelImpl>;

public:
    [[nodiscard]]
    static auto make(std::size_t max_cap) -> std::pair<Sender, Receiver> {
        auto counter = std::make_shared<ChannelImpl>(1, 1, max_cap);
        return std::make_pair(Sender{counter}, Receiver{counter});
    }
};

} // namespace zedio::sync
