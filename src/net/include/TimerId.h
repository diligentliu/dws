#pragma once

#include <cstdint>

#include "copyable.h"

namespace dws::net {

class Timer;

class TimerId : public copyable {
 private:
    Timer* timer_;
    int64_t sequence_;

 public:
    TimerId(Timer* timer, int64_t sequence) : timer_(timer), sequence_(sequence) {}

    TimerId() : timer_(nullptr), sequence_(0) {}

    friend class TimerQueue;
};

}  // namespace dws::net
