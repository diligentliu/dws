#pragma once

#include <vector>

#include "Poller.h"

struct pollfd;

namespace dws::net {

class PollPoller : public Poller {
 public:
    explicit PollPoller(EventLoop* loop);
    ~PollPoller() override;

    Timestamp poll(int timeout_ms, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

 private:
    static const int kInitEventListSize = 16;
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

    using PollFdList = std::vector<struct pollfd>;
    PollFdList pollfds_;
};

}  // namespace dws::net
