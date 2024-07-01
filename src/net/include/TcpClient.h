#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "TcpConnection.h"
#include "ThreadAnnotations.h"
#include "noncopyable.h"

namespace dws::net {

class Connector;
using ConnectorPtr = std::shared_ptr<Connector>;

class TcpClient : noncopyable {
 public:
    TcpClient(EventLoop* loop, const InetAddress& serverAddr, std::string name);
    ~TcpClient();

    void connect();
    void disconnect();
    void stop();

    TcpConnectionPtr connection() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return connection_;
    }

    EventLoop* getLoop() const { return loop_; }
    bool retry() const { return retry_; }
    void enableRetry() { retry_ = true; }
    const std::string& name() const { return name_; }

    void setConnectionCallback(ConnectionCallback cb) { connectionCallback_ = std::move(cb); }

    void setMessageCallback(MessageCallback cb) { messageCallback_ = std::move(cb); }

    void setWriteCompleteCallback(WriteCompleteCallback cb) {
        writeCompleteCallback_ = std::move(cb);
    }

 private:
    EventLoop* loop_;
    ConnectorPtr connector_;
    const std::string name_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    std::atomic<bool> retry_;
    std::atomic<bool> connect_;
    int nextConnId_;
    mutable std::mutex mutex_;
    TcpConnectionPtr connection_ GUARDED_BY(mutex_);

    void newConnection(int sockfd);
    void removeConnection(const TcpConnectionPtr& conn);
};

}  // namespace dws::net
