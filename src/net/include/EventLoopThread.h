#pragma once

#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

#include "Thread.h"

namespace dws::net {

class EventLoop;

class EventLoopThread : noncopyable {
 public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;
    EventLoopThread(ThreadInitCallback cb = ThreadInitCallback(),
                    const std::string& name = std::string());
    ~EventLoopThread();
    EventLoop* startLoop();

 private:
    void threadFunc();

    bool exiting_;
    Thread thread_;
    mutable std::mutex mutex_;
    ThreadInitCallback callback_;
    EventLoop* loop_ GUARDED_BY(mutex_);
    mutable std::condition_variable cond_ GUARDED_BY(mutex_);
};

}  // namespace dws::net
