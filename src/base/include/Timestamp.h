#pragma once

#include <utility>

#include "Types.h"
#include "copyable.h"

namespace dws {

class Timestamp : public dws::copyable {
 private:
    int64_t microSecondsSinceEpoch_;
 public:
    Timestamp() : microSecondsSinceEpoch_(0) { }
    explicit Timestamp(int64_t microSecondsSinceEpochArgs)
        : microSecondsSinceEpoch_(microSecondsSinceEpochArgs) { }
    void swap(Timestamp& other) {
        std::swap(microSecondsSinceEpoch_, other.microSecondsSinceEpoch_);
    }

    std::string toString() const;
    std::string toFormattedString(bool showMicroseconds = true) const;
    bool valid() const { return microSecondsSinceEpoch_ > 0; }
    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
    time_t secondsSinceEpoch() const { return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond); }

    static const int kMicroSecondsPerSecond = 1000 * 1000;
    static Timestamp now();
    static Timestamp invalid() {
        return Timestamp();
    }
    static Timestamp fromUnixTime(time_t t, int microseconds) {
        return Timestamp(static_cast<int64_t>(t) * kMicroSecondsPerSecond + microseconds);
    }
    static Timestamp fromUnixTime(time_t t) {
        return fromUnixTime(t, 0);
    }
};

inline bool operator<(Timestamp lhs, Timestamp rhs) {
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator>(Timestamp lhs, Timestamp rhs) {
    return lhs.microSecondsSinceEpoch() > rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs) {
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

inline bool operator!=(Timestamp lhs, Timestamp rhs) {
    return !(lhs == rhs);
}

inline bool operator<=(Timestamp lhs, Timestamp rhs) {
    return !(rhs < lhs);
}

inline bool operator>=(Timestamp lhs, Timestamp rhs) {
    return !(lhs < rhs);
}

inline double timeDifference(Timestamp high, Timestamp low) {
    int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
    return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}

inline Timestamp addTime(Timestamp timestamp, double seconds) {
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}

}  // namespace dws
