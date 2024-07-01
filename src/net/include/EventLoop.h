#pragma once

#include <any>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "Callbacks.h"
#include "CurrentThread.h"
#include "ThreadAnnotations.h"
#include "TimerId.h"
#include "Timestamp.h"
#include "noncopyable.h"

namespace dws::net {

class Channel;
class Poller;
class TimerQueue;

class EventLoop : noncopyable {
 public:
    using Functor = std::function<void()>;

 private:
    using ChannelList = std::vector<Channel*>;
    std::atomic<bool> looping_;
    std::atomic<bool> quit_;
    std::atomic<bool> eventHandling_;
    std::atomic<bool> callingPendingFunctors_;
    int64_t iteration_;
    const pid_t threadId_;
    Timestamp pollReturnTime_;
    std::unique_ptr<Poller> poller_;
    std::unique_ptr<TimerQueue> timerQueue_;
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;
    std::any context_;
    ChannelList activeChannels_;
    Channel* currentActiveChannel_;
    mutable std::mutex mutex_;
    std::vector<Functor> pendingFunctors_ GUARDED_BY(mutex_);

    void abortNotInLoopThread();
    void handleRead();
    void doPendingFunctors();
    void printActiveChannels() const;

 public:
    EventLoop();
    ~EventLoop();
    void loop();
    void quit();
    Timestamp pollReturnTime() const { return pollReturnTime_; }
    int64_t iteration() const { return iteration_; }
    void runInLoop(Functor cb);
    void queueInLoop(Functor cb);
    size_t queueSize() const;
    TimerId runAt(Timestamp time, TimerCallback cb);
    TimerId runAfter(double delay, TimerCallback cb);
    TimerId runEvery(double interval, TimerCallback cb);
    void cancel(TimerId timerId);
    void wakeup();
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    void assertInLoopThread() {
        if (!isInLoopThread()) {
            abortNotInLoopThread();
        }
    }

    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
    bool eventHandling() const { return eventHandling_; }
    void setContext(const std::any& context) { context_ = context; }
    const std::any& getContext() const { return context_; }
    std::any* getMutableContext() { return &context_; }
    static EventLoop* getEventLoopOfCurrentThread();
};

}  // namespace dws::net
