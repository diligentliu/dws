#pragma once

#include <ctime>
#include <memory>
#include <string>

#include "Types.h"
#include "copyable.h"

namespace dws {

struct DateTime {
    int year = 0;
    int8_t month = 0;
    int8_t day = 0;
    int8_t hour = 0;
    int8_t minute = 0;
    int8_t second = 0;

    DateTime() {}
    explicit DateTime(const struct tm &);
    DateTime(int _year, int8_t _month, int8_t _day, int8_t _hour, int8_t _minute, int8_t _second)
        : year(_year), month(_month), day(_day), hour(_hour), minute(_minute), second(_second) {}
    std::string toIsoString() const;
};

class TimeZone : public copyable {
 public:
    struct Data;

 private:
    explicit TimeZone(std::unique_ptr<Data> data);
    std::shared_ptr<Data> data_;
    friend class TimeZoneTestPeer;

 public:
    TimeZone() = default;
    TimeZone(int easeOfUTC, const char *tzname);
    static TimeZone UTC();
    static TimeZone China();
    static TimeZone loadZoneFile(const char *zoneFile);

    bool valid() const { return static_cast<bool>(data_); }

    struct DateTime toLocalTime(int64_t secondsSinceEpoch, int *utc_offset = nullptr) const;
    int64_t fromLocalTime(const struct DateTime &, bool postTransition = false) const;
    static struct DateTime toUTCTime(int64_t secondsSinceEpoch);
    static int64_t fromUTCTime(const struct DateTime &);
};

}  // namespace dws
