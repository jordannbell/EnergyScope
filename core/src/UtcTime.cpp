#include "energy/UtcTime.hpp"

#include <cstdio>

namespace energy {

namespace {

constexpr std::int64_t kSecondsPerDay = 86400;

struct CivilDate {
    int month;
    int day;
};

// Days since 1970-01-01 -> calendar month and day (proleptic Gregorian).
// Algorithm by Howard Hinnant, "civil_from_days".
CivilDate civilFromDays(std::int64_t days) {
    days += 719468;
    const std::int64_t era = (days >= 0 ? days : days - 146096) / 146097;
    const std::int64_t dayOfEra = days - era * 146097;
    const std::int64_t yearOfEra =
        (dayOfEra - dayOfEra / 1460 + dayOfEra / 36524 - dayOfEra / 146096) / 365;
    const std::int64_t dayOfYear =
        dayOfEra - (365 * yearOfEra + yearOfEra / 4 - yearOfEra / 100);
    const std::int64_t mp = (5 * dayOfYear + 2) / 153;
    const int day = static_cast<int>(dayOfYear - (153 * mp + 2) / 5 + 1);
    const int month = static_cast<int>(mp < 10 ? mp + 3 : mp - 9);
    return {month, day};
}

// Floor division so that negative times (before 1970) still work.
std::int64_t floorDiv(std::int64_t a, std::int64_t b) {
    const std::int64_t q = a / b;
    return (a % b != 0 && (a < 0) != (b < 0)) ? q - 1 : q;
}

}  // namespace

std::string formatUtcDayMinute(std::int64_t unixSeconds) {
    const std::int64_t days = floorDiv(unixSeconds, kSecondsPerDay);
    const std::int64_t secondOfDay = unixSeconds - days * kSecondsPerDay;
    const CivilDate date = civilFromDays(days);

    char buf[32];
    std::snprintf(buf, sizeof buf, "%02d/%02d %02d:%02d", date.day, date.month,
                  static_cast<int>(secondOfDay / 3600),
                  static_cast<int>(secondOfDay % 3600 / 60));
    return buf;
}

}  // namespace energy
