#include "TcpClient.h"

#include <cstdio>
#include <utility>

#include "Connector.h"
#include "EventLoop.h"
#include "Logging.h"
#include "SocketsOps.h"

namespace dws::net {
namespace detail {

void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn) {
    loop->queueInLoop([conn] { conn->connectDestroyed(); });
}

void removeConnector(const ConnectorPtr& connector) {}

}  // namespace detail

TcpClient::TcpClient(EventLoop* loop, const InetAddress& serverAddr, std::string name)
    : loop_(CHECK_NOTNULL(loop)),
      connector_(new Connector(loop, serverAddr)),
      name_(std::move(name)),
      connectionCallback_(defaultConnectionCallback),
      messageCallback_(defaultMessageCallback),
      retry_(false),
      connect_(true),
      nextConnId_(1) {
    connector_->setNewConnectionCallback([this](int socket) { newConnection(socket); });
    LOG(INFO) << "[TcpClient::TcpClient] " << name_ << " - connector " << connector_.get();
}

TcpClient::~TcpClient() {
    LOG(INFO) << "[TcpClient::~TcpClient] " << name_ << " - connector " << connector_.get();
    TcpConnectionPtr conn;
    bool unique = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        unique = connector_.unique();
        conn = connection_;
    }

    if (conn) {
        assert(loop_ == conn->getLoop());
        CloseCallback cb = [this](const TcpConnectionPtr& tcp_conn) {
            detail::removeConnection(loop_, tcp_conn);
        };
        loop_->runInLoop([&conn, &cb] { conn->setCloseCallback(cb); });
        if (unique) {
            conn->forceClose();
        }
    } else {
        connector_->stop();
        loop_->runAfter(1, [this] { detail::removeConnector(connector_); });
    }
}

void TcpClient::connect() {
    LOG(INFO) << "[TcpClient::connect] " << name_ << " - connecting to "
              << connector_->serverAddress().toIpPort();
    connect_ = true;
    connector_->start();
}

void TcpClient::disconnect() {
    connect_ = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (connection_) {
            connection_->shutdown();
        }
    }
}

void TcpClient::stop() {
    connect_ = false;
    connector_->stop();
}

void TcpClient::newConnection(int sockfd) {
    loop_->assertInLoopThread();
    InetAddress peerAddr(sockets::getPeerAddr(sockfd));
    char buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;
    InetAddress localAddr(sockets::getLocalAddr(sockfd));
    TcpConnectionPtr conn(new TcpConnection(loop_, connName, sockfd, localAddr, peerAddr));
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
            [this](const TcpConnectionPtr& tcp_conn) { removeConnection(tcp_conn); });
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connection_ = conn;
    }
    conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn) {
    loop_->assertInLoopThread();
    assert(loop_ == conn->getLoop());

    {
        std::lock_guard<std::mutex> lock(mutex_);
        assert(connection_ == conn);
        connection_.reset();
    }

    loop_->queueInLoop(([&conn] { conn->connectDestroyed(); }));
    if (retry_ && connect_) {
        LOG(INFO) << "[TcpClient::connect] " << name_ << " - Reconnecting to "
                  << connector_->serverAddress().toIpPort();
        connector_->restart();
    }
}

}  // namespace dws::net
