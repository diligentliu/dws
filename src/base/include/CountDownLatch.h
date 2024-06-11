#pragma once

#include <condition_variable>
#include <mutex>

#include "Types.h"
#include "ThreadAnnotations.h"
#include "noncopyable.h"

namespace dws {

class CountDownLatch : noncopyable {
 private:
    mutable std::mutex mutex_;
    std::condition_variable condition_ GUARDED_BY(mutex_);
    int count_ GUARDED_BY(mutex_);
 public:
    explicit CountDownLatch(int count);
    void wait();
    void countDown();
    int getCount() const;
};

}  // namespace dws
