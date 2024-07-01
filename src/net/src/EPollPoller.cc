#include "EPollPoller.h"

#include <poll.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>

#include "Channel.h"
#include "Logging.h"

// On Linux, the constants of poll(2) and epoll(4)
// are expected to be the same.
static_assert(EPOLLIN == POLLIN, "epoll uses same flag values as poll");
static_assert(EPOLLPRI == POLLPRI, "epoll uses same flag values as poll");
static_assert(EPOLLOUT == POLLOUT, "epoll uses same flag values as poll");
static_assert(EPOLLRDHUP == POLLRDHUP, "epoll uses same flag values as poll");
static_assert(EPOLLERR == POLLERR, "epoll uses same flag values as poll");
static_assert(EPOLLHUP == POLLHUP, "epoll uses same flag values as poll");

namespace dws::net {
namespace {
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
}  // namespace

EPollPoller::EPollPoller(EventLoop* loop)
    : Poller(loop), epollfd_(::epoll_create1(EPOLL_CLOEXEC)), events_(kInitEventListSize) {
    if (epollfd_ < 0) {
        LOG_SYSFATAL << "[EPollPoller::EPollPoller] epollfd_ < 0";
    }
}

EPollPoller::~EPollPoller() { ::close(epollfd_); }

Timestamp EPollPoller::poll(int timeout_ms, ChannelList* activeChannels) {
    LOG(TRACE) << "[EPollPoller::poll] fd total count " << channels_.size();
    int numEvents =
            ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeout_ms);
    int err = errno;
    Timestamp now(Timestamp::now());
    if (err > 0) {
        LOG(TRACE) << "[EPollPoller::poll]" << numEvents << " events happened";
        fillActiveChannels(numEvents, activeChannels);
        if (static_cast<size_t>(numEvents) == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    } else if (numEvents == 0) {
        LOG(TRACE) << "[EPollPoller::poll] nothing happened";
    } else {
        if (err != EINTR) {
            errno = err;
            LOG_SYSERR << "[EPollPoller::poll] ERROR";
        }
    }
    return now;
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const {
    assert(static_cast<size_t>(numEvents) <= events_.size());
    for (int i = 0; i < numEvents; ++i) {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
#ifndef NDEBUG
        int fd = channel->fd();
        auto it = channels_.find(fd);
        assert(it != channels_.end());
        assert(it->second == channel);
#endif
        channel->set_revents(events_[i].events);
        activeChannels->emplace_back(channel);
    }
}

void EPollPoller::updateChannel(Channel* channel) {
    Poller::assertInLoopThread();
    const int index = channel->index();
    const int fd = channel->fd();
    LOG(TRACE) << "[EPollPoller::updateChannel] fd = " << fd << " events = " << channel->events()
               << " index = " << index;
    if (index == kNew || index == kDeleted) {
        if (index == kNew) {
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        } else {
            auto it = channels_.find(fd);
            assert(it != channels_.end());
            assert(it->second == channel);
        }

        channel->set_index(index);
        update(EPOLL_CTL_ADD, channel);
    } else {
        auto it = channels_.find(fd);
        assert(it != channels_.end());
        assert(it->second == channel);
        assert(index == kAdded);
        if (channel->isNoneEvent()) {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EPollPoller::removeChannel(Channel* channel) {
    Poller::assertInLoopThread();
    int fd = channel->fd();
    LOG(TRACE) << "[EPollPoller::removeChannel] fd = " << fd;
    auto it = channels_.find(fd);
    assert(it != channels_.end());
    assert(it->second == channel);
    assert(channel->isNoneEvent());
    int index = channel->index();
    assert(index == kAdded || index == kDeleted);
    size_t n = channels_.erase(fd);
    assert(n == 1);

    if (index == kAdded) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

void EPollPoller::update(int operation, Channel* channel) {
    struct epoll_event event {};
    bzero(&event, sizeof event);
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    LOG(TRACE) << "[EPollPoller::update] epoll_ctl op = " << operationToString(operation)
               << " fd = " << fd << " event = { " << channel->eventsToString() << "}";
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
        if (operation == EPOLL_CTL_DEL) {
            LOG_SYSERR << "[EPollPoller::update]"
                       << "epoll_ctl op = " << operationToString(operation);
        } else {
            LOG_SYSFATAL << "[EPollPoller::update]"
                         << "epoll_ctl op = " << operationToString(operation);
        }
    }
}

const char* EPollPoller::operationToString(int operation) {
    switch (operation) {
        case EPOLL_CTL_ADD:
            return "ADD";
        case EPOLL_CTL_DEL:
            return "DEL";
        case EPOLL_CTL_MOD:
            return "MOD";
        default:
            assert(false && "ERROR op");
            return "Unknown Operation";
    }
}

}  // namespace dws::net
