#pragma once

#include <cassert>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <utility>

#include "Types.h"
#include "noncopyable.h"
#include "ThreadAnnotations.h"

namespace dws {

template <typename T>
class BoundedBlockingQueue : noncopyable {
    using queue_type = std::deque<T>;

 private:
    mutable std::mutex mutex_;
    std::condition_variable notEmpty_ GUARDED_BY(mutex_);
    std::condition_variable notFull_ GUARDED_BY(mutex_);
    const std::size_t max_size_;
    queue_type queue_ GUARDED_BY(mutex_);

 public:
    explicit BoundedBlockingQueue(size_t maxSize)
        : mutex_(), notEmpty_(), notFull_(), max_size_(maxSize), queue_() {
        queue_.reserve(maxSize);
    }

    void put(const T &x) {
        std::unique_lock<std::mutex> lock(mutex_);
        // 这里写成 while 是为了防止伪唤醒
        while (queue_.size() >= max_size_) {
            notFull_.wait(lock);
        }
        assert(queue_.size() < max_size_);
        queue_.emplace_back(x);
        notEmpty_.notify_one();
    }

    void put(T &&x) {
        std::unique_lock<std::mutex> lock(mutex_);
        while (queue_.size() >= max_size_) {
            notFull_.wait(lock);
        }
        assert(queue_.size() < max_size_);
        queue_.emplace_back(std::move(x));
        notEmpty_.notify_one();
    }

    T get() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (queue_.empty()) {
            notEmpty_.wait(lock);
        }
        assert(!queue_.empty());
        T front(std::move(queue_.front()));
        queue_.pop_front();
        notFull_.notify_one();
        return front;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    bool full() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size() >= max_size_;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    size_t capacity() const {
        return max_size_;
    }
};

}  // namespace dws
