#include "TcpConnection.h"

#include <cerrno>

#include "Channel.h"
#include "EventLoop.h"
#include "Logging.h"
#include "Socket.h"
#include "SocketsOps.h"
#include "WeakCallback.h"

namespace dws::net {

void defaultConnectionCallback(const TcpConnectionPtr& conn) {
    LOG(TRACE) << conn->localAddress().toIpPort() << " -> " << conn->peerAddress().toIpPort()
               << " is " << (conn->connected() ? "UP" : "DOWN");
}

void defaultMessageCallback(const TcpConnectionPtr&, Buffer* buffer, Timestamp) {
    buffer->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop* loop, const std::string& name, int sockfd,
                             const InetAddress& localAddr, const InetAddress& peerAddr)
    : loop_(CHECK_NOTNULL(loop)),
      name_(name),
      state_(kConnecting),
      reading_(true),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64 * 1024 * 1024) {
    channel_->setReadCallback([this](Timestamp timestamp) { handleRead(timestamp); });
    channel_->setWriteCallback([this] { handleWrite(); });
    channel_->setCloseCallback([this] { handleClose(); });
    channel_->setErrorCallback([this] { handleError(); });
    LOG(DEBUG) << "[TcpConnection::TcpConnection]" << " TcpConnection::ctor[" << name_ << "] at "
               << this << " fd = " << sockfd;
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
    LOG(DEBUG) << "[TcpConnection::~TcpConnection]" << " TcpConnection::dtor[" << name_ << "] at "
               << this << " fd = " << channel_->fd() << " state = " << stateToString();
    assert(state_ == kDisconnected);
}

bool TcpConnection::getTcpInfo(struct tcp_info* tcpi) const { return socket_->getTcpInfo(tcpi); }

std::string TcpConnection::getTcpInfoString() const {
    char buf[1024];
    bzero(buf, 1024);
    socket_->getTcpInfoString(buf, sizeof buf);
    return buf;
}

void TcpConnection::send(const void* message, int len) {
    send(StringPiece(reinterpret_cast<const char*>(message), len));
}

void TcpConnection::send(const StringPiece& message) {
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(message);
        } else {
            loop_->runInLoop([this, message] { sendInLoop(message); });
        }
    }
}

void TcpConnection::send(Buffer* buf) {
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(buf->peek(), buf->readableBytes());
            buf->retrieveAll();
        } else {
            loop_->runInLoop([this, buf] { sendInLoop(buf->toStringPiece()); });
        }
    }
}

void TcpConnection::sendInLoop(const StringPiece& message) {
    sendInLoop(message.data(), message.size());
}

void TcpConnection::sendInLoop(const void* message, size_t len) {
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool err = false;
    if (state_ == kDisconnected) {
        LOG(WARN) << "[TcpConnection::sendInLoop]" << " disconnected, give up writing";
        return;
    }
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        nwrote = sockets::write(channel_->fd(), message, len);
        if (nwrote >= 0) {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_) {
                loop_->queueInLoop([this] { writeCompleteCallback_(shared_from_this()); });
            }
        } else {
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                LOG_SYSERR << "[TcpConnection::sendInLoop] ERROR";
                if (errno == EPIPE || errno == ECONNRESET) {
                    err = true;
                }
            }
        }
    }

    assert(remaining <= len);
    if (!err && remaining > 0) {
        size_t oldLen = outputBuffer_.readableBytes();
        size_t size = oldLen + remaining;
        if (size >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_) {
            loop_->queueInLoop([this, &size] { highWaterMarkCallback_(shared_from_this(), size); });
            outputBuffer_.append(reinterpret_cast<const char*>(message) + nwrote, remaining);
            if (!channel_->isWriting()) {
                channel_->enableWriting();
            }
        }
    }
}

void TcpConnection::shutdown() {
    if (state_ == kConnected) {
        setState(kDisconnecting);
        loop_->runInLoop([this] { shutdownInLoop(); });
    }
}

void TcpConnection::shutdownInLoop() {
    loop_->assertInLoopThread();
    if (!channel_->isWriting()) {
        socket_->shutdownWrite();
    }
}

void TcpConnection::forceClose() {
    if (state_ == kConnected || state_ == kDisconnecting) {
        setState(kDisconnecting);
        loop_->queueInLoop([ptr = shared_from_this()] { ptr->forceCloseInLoop(); });
    }
}

void TcpConnection::forceCloseWithDelay(double seconds) {
    loop_->assertInLoopThread();
    if (state_ == kConnected || state_ == kDisconnecting) {
        setState(kDisconnecting);
        loop_->runAfter(seconds, makeWeakCallback(shared_from_this(), &TcpConnection::forceClose));
    }
}

void TcpConnection::forceCloseInLoop() {
    loop_->assertInLoopThread();
    if (state_ == kConnected || state_ == kDisconnecting) {
        handleClose();
    }
}

const char* TcpConnection::stateToString() const {
    switch (state_) {
        case kDisconnected:
            return "kDisconnected";
        case kConnecting:
            return "kConnecting";
        case kConnected:
            return "kConnected";
        case kDisconnecting:
            return "kDisconnecting";
        default:
            return "unknown state";
    }
}

void TcpConnection::setTcpNoDelay(bool on) { socket_->setTcpNoDelay(on); }

void TcpConnection::startRead() {
    loop_->runInLoop([this] { startReadInLoop(); });
}

void TcpConnection::startReadInLoop() {
    loop_->assertInLoopThread();
    if (!reading_ || !channel_->isReading()) {
        channel_->enableReading();
        reading_ = true;
    }
}

void TcpConnection::stopRead() {
    loop_->runInLoop([this] { stopReadInLoop(); });
}

void TcpConnection::stopReadInLoop() {
    loop_->assertInLoopThread();
    if (reading_ || channel_->isReading()) {
        channel_->disableReading();
        reading_ = false;
    }
}

void TcpConnection::connectEstablished() {
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();

    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed() {
    loop_->assertInLoopThread();
    if (state_ == kConnected) {
        setState(kDisconnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}

void TcpConnection::handleRead(Timestamp receiveTime) {
    loop_->assertInLoopThread();
    int err = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &err);
    if (n > 0) {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    } else if (n == 0) {
        handleClose();
    } else {
        errno = err;
        LOG_SYSERR << "[TcpConnection::handleRead] ERROR";
        handleError();
    }
}

void TcpConnection::handleWrite() {
    loop_->assertInLoopThread();
    if (channel_->isWriting()) {
        ssize_t n =
                sockets::write(channel_->fd(), outputBuffer_.peek(), outputBuffer_.readableBytes());
        if (n > 0) {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0) {
                channel_->disableWriting();
                if (writeCompleteCallback_) {
                    loop_->queueInLoop([this] { writeCompleteCallback_(shared_from_this()); });
                }
                if (state_ == kDisconnecting) {
                    shutdownInLoop();
                }
            }
        } else {
            LOG_SYSERR << "[TcpConnection::handleWrite] ERROR";
        }
    } else {
        LOG(TRACE) << "[TcpConnection::handleWrite]" << "Connection fd = " << channel_->fd()
                   << " is down, no more writing";
    }
}

void TcpConnection::handleClose() {
    loop_->assertInLoopThread();
    LOG(TRACE) << "[TcpConnection::handleClose]" << " fd = " << channel_->fd()
               << " state = " << stateToString();
    assert(state_ == kConnected || state_ == kDisconnecting);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr guardThis(shared_from_this());
    connectionCallback_(guardThis);
    closeCallback_(guardThis);
}

void TcpConnection::handleError() {
    int err = sockets::getSocketError(channel_->fd());
    LOG(ERROR) << "[TcpConnection::handleError] " << name_ << " - SO_ERROR = " << err << " "
               << strerror_tl(err);
}

}  // namespace dws::net
