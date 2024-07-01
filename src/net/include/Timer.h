#pragma once

#include <atomic>
#include <utility>

#include "Callbacks.h"
#include "Timestamp.h"
#include "noncopyable.h"

namespace dws::net {

class Timer : noncopyable {
 private:
    const TimerCallback callback_;
    Timestamp expiration_;
    const double interval_;
    const bool repeat_;
    const int64_t sequence_;

    static std::atomic<int64_t> s_numCreated_;

 public:
    Timer(TimerCallback callback, Timestamp when, double interval)
        : callback_(std::move(callback)),
          expiration_(when),
          interval_(interval),
          repeat_(interval > 0.0),
          sequence_(s_numCreated_.fetch_add(1)) {}

    void run() const { callback_(); }

    Timestamp expiration() const { return expiration_; }
    bool repeat() const { return repeat_; }
    int64_t sequence() const { return sequence_; }

    void restart(Timestamp now);

    static int64_t numCreated() { return s_numCreated_.load(); }
};

}  // namespace dws::net
