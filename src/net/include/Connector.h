#pragma once

#include <atomic>
#include <functional>
#include <memory>

#include "InetAddress.h"
#include "noncopyable.h"

namespace dws::net {

class Channel;
class EventLoop;

class Connector : noncopyable, public std::enable_shared_from_this<Connector> {
 public:
    using NewConnectionCallback = std::function<void(int sockfd)>;

    Connector(EventLoop* loop, const InetAddress& serverAddr);
    ~Connector();

    void setNewConnectionCallback(const NewConnectionCallback& cb) { newConnectionCallback_ = cb; }

    void start();
    void restart();
    void stop();

    const InetAddress& serverAddress() const { return serverAddr_; }

 private:
    enum State { kDisconnected, kConnecting, kConnected };
    static const int kMaxRetryDelayMs = 30 * 1000;
    static const int kInitRetryDelayMs = 500;

    void setState(State s) { state_ = s; }
    void startInLoop();
    void stopInLoop();
    void connect();
    void connecting(int sockfd);
    void handleWrite();
    void handleError();
    void retry(int sockfd);
    int removeAndResetChannel();
    void resetChannel();

    EventLoop* loop_;
    InetAddress serverAddr_;
    std::atomic<bool> connect_;
    State state_;
    std::unique_ptr<Channel> channel_;
    NewConnectionCallback newConnectionCallback_;
    int retryDelayMs_;
};

}  // namespace dws::net
