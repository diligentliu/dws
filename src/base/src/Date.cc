#include "Date.h"

#include <cstdio>  // snprintf
#include <ctime>   // struct tm

namespace dws {
namespace detail {
// 儒略日计算
int getJulianDayNumber(int year, int month, int day) {
    int a = (14 - month) / 12;
    int y = year + 4800 - a;
    int m = month + 12 * a - 3;
    return day + (153 * m + 2) / 5 + y * 365 + y / 4 - y / 100 + y / 400 - 32045;
}

struct Date::YearMonthDay getYearMonthDay(int julianDayNumber) {
    int a = julianDayNumber + 32044;
    int b = (4 * a + 3) / 146097;
    int c = a - ((b * 146097) / 4);
    int d = (4 * c + 3) / 1461;
    int e = c - ((1461 * d) / 4);
    int m = (5 * e + 2) / 153;
    Date::YearMonthDay ymd;
    ymd.day = static_cast<int8_t>(e - ((153 * m + 2) / 5) + 1);
    ymd.month = static_cast<int8_t>(m + 3 - 12 * (m / 10));
    ymd.year = b * 100 + d - 4800 + (m / 10);
    return ymd;
}
}  // namespace detail

const int Date::kJulianDayOf1970_01_01 = detail::getJulianDayNumber(1970, 1, 1);

Date::Date(int year, int month, int day)
    : julianDayNumber_(detail::getJulianDayNumber(year, month, day)) {}

Date::Date(const struct tm &t)
    : julianDayNumber_(detail::getJulianDayNumber(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday)) {}

Date::YearMonthDay Date::yearMonthDay() const { return detail::getYearMonthDay(julianDayNumber_); }

std::string Date::toIsoString() const {
    char buf[32];
    YearMonthDay ymd(yearMonthDay());
    snprintf(buf, sizeof buf, "%04d-%02d-%02d", ymd.year, ymd.month, ymd.day);
    return buf;
}
}  // namespace dws
