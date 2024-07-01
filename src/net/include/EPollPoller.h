#pragma once

#include <vector>

#include "Poller.h"

struct epoll_event;

namespace dws::net {

class EPollPoller : public Poller {
 public:
    explicit EPollPoller(EventLoop* loop);
    ~EPollPoller() override;

    Timestamp poll(int timeout_ms, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

 private:
    static const int kInitEventListSize = 16;
    static const char* operationToString(int op);
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    void update(int operation, Channel* channel);

    using EventList = std::vector<struct epoll_event>;

    int epollfd_;
    EventList events_;
};

}  // namespace dws::net
