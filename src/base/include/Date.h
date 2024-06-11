#pragma once

#include "Types.h"
#include "copyable.h"

struct tm;

namespace dws {

class Date : public copyable {
 private:
    int julianDayNumber_;
 public:
    struct YearMonthDay {
        int year;
        int8_t month;
        int8_t day;
    };

    static const int8_t kDaysPerWeek = 7;
    static const int kJulianDayOf1970_01_01;

    Date() : julianDayNumber_(0) {}
    Date(int year, int month, int day);
    explicit Date(int julianDayNum) : julianDayNumber_(julianDayNum) {}
    explicit Date(const struct tm &);

    void swap(Date &other) {
        std::swap(julianDayNumber_, other.julianDayNumber_);
    }

    bool valid() const {
        return julianDayNumber_ > 0;
    }

    std::string toIsoString() const;
    YearMonthDay yearMonthDay() const;
    int julianDayNumber() const { return julianDayNumber_; }

    int year() const {
        return yearMonthDay().year;
    }

    int8_t month() const {
        return yearMonthDay().month;
    }

    int8_t day() const {
        return yearMonthDay().day;
    }

    int8_t weekDay() const {
        return static_cast<int8_t>((julianDayNumber_ + 1) % kDaysPerWeek);
    }
};

inline bool operator<(Date x, Date y) {
    return x.julianDayNumber() < y.julianDayNumber();
}

inline bool operator>(Date x, Date y) {
    return x.julianDayNumber() > y.julianDayNumber();
}

inline bool operator==(Date x, Date y) {
    return x.julianDayNumber() == y.julianDayNumber();
}

inline bool operator>=(Date x, Date y) {
    return x.julianDayNumber() >= y.julianDayNumber();
}

inline bool operator<=(Date x, Date y) {
    return x.julianDayNumber() <= y.julianDayNumber();
}

inline bool operator!=(Date x, Date y) {
    return x.julianDayNumber() != y.julianDayNumber();
}

}  // namespace dws
