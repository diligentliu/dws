#pragma once

#include <string>
#include <atomic>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>

#include "noncopyable.h"
#include "Types.h"
#include "BlockQueue.h"
#include "BoundedBlockQueue.h"
#include "CountDownLatch.h"
#include "LogStream.h"
#include "Thread.h"
#include "ThreadAnnotations.h"

namespace dws {

class AsyncLogging : noncopyable {
 private:
    void threadFunc();

    using Buffer = detail::FixedBuffer<detail::kLargeBuffer>;
    using BufferVector = std::vector<std::unique_ptr<Buffer>>;
    using BufferPtr = BufferVector::value_type;

    const int flushInterval_;
    std::atomic<bool> running_;
    const std::string basename_;
    const off_t rollSize_;
    Thread thread_;
    CountDownLatch latch_;
    std::mutex mutex_;
    std::condition_variable cond_ GUARDED_BY(mutex_);
    BufferPtr currentBuffer_ GUARDED_BY(mutex_);
    BufferPtr nextBuffer_ GUARDED_BY(mutex_);
    BufferVector buffers_ GUARDED_BY(mutex_);

 public:
    AsyncLogging(const std::string &basename, off_t rollSize, int flushInterval = 3);
    ~AsyncLogging() {
        if (running_) {
            stop();
        }
    }

    void start() {
        running_ = true;
        thread_.start();
        latch_.wait();
    }

    void stop() NO_THREAD_SAFETY_ANALYSIS {
        running_ = false;
        cond_.notify_one();
        thread_.join();
    }

    void append(const char *logline, int len);
};

}  // namespace dws
