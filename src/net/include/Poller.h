#pragma once

#include <unordered_map>
#include <vector>

#include "EventLoop.h"
#include "Timestamp.h"

namespace dws::net {

class Channel;

class Poller : noncopyable {
 public:
    using ChannelList = std::vector<Channel*>;

    explicit Poller(EventLoop* loop);
    virtual ~Poller() = default;

    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;
    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;
    virtual bool hasChannel(Channel* channel) const;

    static Poller* newDefaultPoller(EventLoop* loop);

    void assertInLoopThread() const { ownerLoop_->assertInLoopThread(); }

 protected:
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;

 private:
    EventLoop* ownerLoop_;
};

}  // namespace dws::net
