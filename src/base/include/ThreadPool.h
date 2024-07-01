#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "Thread.h"
#include "ThreadAnnotations.h"
#include "Types.h"
#include "noncopyable.h"

namespace dws {

class ThreadPool : noncopyable {
 private:
    using Task = std::function<void()>;
    mutable std::mutex mutex_;
    std::condition_variable notEmpty_ GUARDED_BY(mutex_);
    std::condition_variable notFull_ GUARDED_BY(mutex_);
    std::string name_;
    Task threadInitCallback_;
    std::vector<std::unique_ptr<Thread>> threads_;
    std::deque<Task> queue_ GUARDED_BY(mutex_);
    uint64_t maxQueueSize_;
    bool running_;

    bool isFull() const REQUIRES(mutex_);
    void runInThread();
    Task take();

 public:
    explicit ThreadPool(std::string name = std::string("ThreadPool"));
    ~ThreadPool();

    void setMaxQueueSize(uint64_t maxSize) { maxQueueSize_ = maxSize; }
    void setThreadInitCallback(const Task& cb) { threadInitCallback_ = cb; }

    void start(int numThreads);
    void stop();
    const std::string& name() const { return name_; }

    uint64_t queueSize() const REQUIRES(mutex_);
    void run(Task f);
};  // class ThreadPool

}  // namespace dws
