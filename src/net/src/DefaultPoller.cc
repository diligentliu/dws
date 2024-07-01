#include <cstdlib>

#include "EPollPoller.h"
#include "PollPoller.h"
#include "Poller.h"

namespace dws::net {

Poller* Poller::newDefaultPoller(dws::net::EventLoop* loop) {
    if (::getenv("DWS_USE_POLL")) {
        return new PollPoller(loop);
    } else {
        return new EPollPoller(loop);
    }
}

}  // namespace dws::net
