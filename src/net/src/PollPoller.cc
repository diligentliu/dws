#include "PollPoller.h"

#include <poll.h>

#include <cerrno>

#include "Channel.h"
#include "Logging.h"
#include "Types.h"

namespace dws::net {

PollPoller::PollPoller(EventLoop* loop) : Poller(loop), pollfds_(kInitEventListSize) {}

PollPoller::~PollPoller() = default;

Timestamp PollPoller::poll(int timeout_ms, Poller::ChannelList* activeChannels) {
    int numEvents = ::poll(&*pollfds_.begin(), pollfds_.size(), timeout_ms);
    int err = errno;
    Timestamp now(Timestamp::now());
    if (numEvents > 0) {
        LOG(TRACE) << "[PollPoller::poll]" << numEvents << " events happened";
        fillActiveChannels(numEvents, activeChannels);
    } else if (numEvents == 0) {
        LOG(TRACE) << "[PollPoller::poll] nothing happened";
    } else {
        if (err == EINTR) {
            errno = err;
            LOG_SYSERR << "[PollPoller::poll] ERROR";
        }
    }
    return now;
}

void PollPoller::fillActiveChannels(int numEvents, Poller::ChannelList* activeChannels) const {
    for (auto pfd = pollfds_.begin(); pfd != pollfds_.end() && numEvents > 0; ++pfd) {
        if (pfd->revents > 0) {
            --numEvents;
            auto it = channels_.find(pfd->fd);
            assert(it != channels_.end());
            Channel* channel = it->second;
            assert(channel->fd() == pfd->fd);
            channel->set_revents(pfd->revents);
            activeChannels->emplace_back(channel);
        }
    }
}

void PollPoller::updateChannel(Channel* channel) {
    Poller::assertInLoopThread();
    int fd = channel->fd();
    int events = channel->events();
    LOG(TRACE) << "[PollPoller::updateChannel]" << " fd = " << fd << " events = " << events;
    if (channel->index() < 0) {
        assert(channels_.find(fd) == channels_.end());
        struct pollfd pfd {};
        pfd.fd = fd;
        pfd.events = static_cast<int16_t>(events);
        pfd.revents = 0;
        pollfds_.emplace_back(pfd);
        int idx = static_cast<int>(pollfds_.size() - 1);
        channel->set_index(idx);
        channels_[fd] = channel;
    } else {
        auto it = channels_.find(fd);
        assert(it != channels_.end());
        assert(it->second == channel);
        int index = channel->index();
        assert(index >= 0 && index < static_cast<int>(pollfds_.size()));
        struct pollfd& pfd = pollfds_[index];
        assert(pfd.fd == channel->fd() || pfd.fd == -fd - 1);
        pfd.fd = fd;
        pfd.events = static_cast<int16_t>(events);
        pfd.revents = 0;
        if (channel->isNoneEvent()) {
            pfd.fd = -fd - 1;
        }
    }
}

void PollPoller::removeChannel(Channel* channel) {
    Poller::assertInLoopThread();
    int fd = channel->fd();
    LOG(TRACE) << "[PollPoller::removeChannel]" << " fd = " << fd;
    auto it = channels_.find(fd);
    assert(it != channels_.end());
    assert(it->second == channel);
    assert(channel->isNoneEvent());
    int index = channel->index();
    assert(index >= 0 && index < static_cast<int>(pollfds_.size()));
    const struct pollfd& pfd = pollfds_[index];
    assert(pfd.fd == -fd - 1 && pfd.events == channel->events());
    channels_.erase(it);
    if (static_cast<size_t>(index) == pollfds_.size() - 1) {
        pollfds_.pop_back();
    } else {
        int channelAtEnd = pollfds_.back().fd;
        std::iter_swap(pollfds_.begin() + index, pollfds_.end() - 1);
        if (channelAtEnd < 0) {
            channelAtEnd = -channelAtEnd - 1;
        }
        channels_[channelAtEnd]->set_index(index);
        pollfds_.pop_back();
    }
}

}  // namespace dws::net
