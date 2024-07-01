#include "Connector.h"

#include <cerrno>

#include "Channel.h"
#include "EventLoop.h"
#include "Logging.h"
#include "SocketsOps.h"

namespace dws::net {

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
    : loop_(loop),
      serverAddr_(serverAddr),
      connect_(false),
      state_(kDisconnected),
      retryDelayMs_(kInitRetryDelayMs) {
    LOG(DEBUG) << "ctor[" << this << "]";
}

Connector::~Connector() {
    LOG(DEBUG) << "dtor[" << this << "]";
    assert(!channel_);
}

void Connector::start() {
    connect_ = true;
    loop_->runInLoop([this]() { startInLoop(); });
}

void Connector::startInLoop() {
    loop_->assertInLoopThread();
    assert(state_ == kDisconnected);
    if (connect_) {
        connect();
    } else {
        LOG(DEBUG) << "[Connector::startInLoop] don't connect";
    }
}

void Connector::stop() {
    connect_ = false;
    loop_->queueInLoop([this]() { stopInLoop(); });
}

void Connector::stopInLoop() {
    loop_->assertInLoopThread();
    if (state_ == kConnecting) {
        setState(kDisconnected);
        int sockfd = removeAndResetChannel();
        retry(sockfd);
    }
}

void Connector::connect() {
    int sockfd = sockets::createNonblockingOrDie(serverAddr_.family());
    int res = sockets::connect(sockfd, serverAddr_.getSockAddr());
    int save_errno = (res == 0) ? 0 : errno;
    switch (save_errno) {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN:
            connecting(sockfd);
            break;

        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
            retry(sockfd);
            break;

        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            LOG_SYSERR << "[Connector::connect] connect error" << save_errno;
            sockets::close(sockfd);
            break;

        default:
            LOG_SYSERR << "[Connector::connect] Unexpected error" << save_errno;
            sockets::close(sockfd);
            break;
    }
}

void Connector::restart() {
    loop_->assertInLoopThread();
    setState(kDisconnected);
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}

void Connector::connecting(int sockfd) {
    setState(kConnecting);
    assert(!channel_);
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallback([this]() { handleWrite(); });
    channel_->setErrorCallback([this]() { handleError(); });
    channel_->enableWriting();
}

int Connector::removeAndResetChannel() {
    channel_->disableAll();
    channel_->remove();
    int sockfd = channel_->fd();
    loop_->queueInLoop([this]() { resetChannel(); });
    return sockfd;
}

void Connector::resetChannel() { channel_.reset(); }

void Connector::handleWrite() {
    LOG(TRACE) << "[Connector::handleWrite] state " << state_;
    if (state_ == kConnecting) {
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        if (err) {
            LOG(WARN) << "[Connector::handleWrite] SOERROR = " << err << " " << strerror_tl(err);
            retry(sockfd);
        } else if (sockets::isSelfConnect(sockfd)) {
            LOG(WARN) << "[Connector::handleWrite] Self connect";
            retry(sockfd);
        } else {
            setState(kConnected);
            if (connect_) {
                newConnectionCallback_(sockfd);
            } else {
                sockets::close(sockfd);
            }
        }
    } else {
        assert(state_ == kDisconnected);
    }
}

void Connector::handleError() {
    LOG(ERROR) << "[Connector::handleError] state = " << state_;
    if (state_ == kConnecting) {
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        LOG(TRACE) << "SO_ERROR = " << err << " " << strerror_tl(err);
        retry(sockfd);
    }
}

void Connector::retry(int sockfd) {
    sockets::close(sockfd);
    setState(kDisconnected);
    if (connect_) {
        LOG(INFO) << "[Connector::retry] Retry connecting to " << serverAddr_.toIpPort() << " in "
                  << retryDelayMs_ << " milliseconds.";
        loop_->runAfter(retryDelayMs_ / 1000.0,
                        [ptr = shared_from_this()] { ptr->startInLoop(); });
        retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
    } else {
        LOG(DEBUG) << "[Connector::retry] don't connect";
    }
}

}  // namespace dws::net
