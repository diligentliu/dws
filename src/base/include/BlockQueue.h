#pragma once

#include <cassert>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <utility>

#include "ThreadAnnotations.h"
#include "noncopyable.h"

namespace dws {

template <typename T>
class BlockQueue : noncopyable {
    using queue_type = std::deque<T>;

 private:
    mutable std::mutex mutex_;
    std::condition_variable notEmpty_ GUARDED_BY(mutex_);
    queue_type queue_ GUARDED_BY(mutex_);

 public:
    BlockQueue() : mutex_(), notEmpty_(), queue_() {}

    void put(const T &x) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.emplace_back(x);
        notEmpty_.notify_one();
    }

    void put(T &&x) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.emplace_back(std::move(x));
        notEmpty_.notify_one();
    }

    T take() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (queue_.empty()) {
            notEmpty_.wait(lock);
        }
        assert(!queue_.empty());
        T front(std::move(queue_.front()));
        queue_.pop_front();
        return front;
    }

    queue_type drain() {
        queue_type queue;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue.swap(queue_);
            assert(queue_.empty());
        }
        return queue;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
};  // __attribute__((aligned(64)));

}  // namespace dws
