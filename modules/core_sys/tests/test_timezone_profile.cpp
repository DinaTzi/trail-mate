#include "platform/ui/timezone_profile.h"

#include <cassert>
#include <ctime>
#include <string>

namespace
{

std::time_t utc_epoch(int year, int month, int day, int hour, int minute, int second)
{
    std::tm tm{};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
#if defined(_WIN32)
    return _mkgmtime(&tm);
#else
    return timegm(&tm);
#endif
}

} // namespace

int main()
{
    using namespace platform::ui::time;

    const auto* eastern = timezone_profile_by_tzdef("EST5EDT,M3.2.0/2,M11.1.0/2");
    assert(eastern != nullptr);
    assert(timezone_offset_for_profile_at(*eastern, utc_epoch(2026, 1, 15, 12, 0, 0)) == -300);
    assert(timezone_offset_for_profile_at(*eastern, utc_epoch(2026, 5, 22, 12, 0, 0)) == -240);
    assert(timezone_offset_for_profile_at(*eastern, utc_epoch(2026, 11, 15, 12, 0, 0)) == -300);
    assert(timezone_offset_for_profile_at(*eastern, utc_epoch(2026, 3, 8, 6, 59, 59)) == -300);
    assert(timezone_offset_for_profile_at(*eastern, utc_epoch(2026, 3, 8, 7, 0, 0)) == -240);
    assert(timezone_offset_for_profile_at(*eastern, utc_epoch(2026, 11, 1, 5, 59, 59)) == -240);
    assert(timezone_offset_for_profile_at(*eastern, utc_epoch(2026, 11, 1, 6, 0, 0)) == -300);
    assert(timezone_profile_id_for_legacy_offset(-300) == eastern->id);

    const auto* central = timezone_profile_by_tzdef("CST6CDT,M3.2.0/2,M11.1.0/2");
    assert(central != nullptr);
    assert(timezone_offset_for_profile_at(*central, utc_epoch(2026, 5, 22, 12, 0, 0)) == -300);
    assert(timezone_offset_for_profile_at(*central, utc_epoch(2026, 3, 8, 7, 59, 59)) == -360);
    assert(timezone_offset_for_profile_at(*central, utc_epoch(2026, 3, 8, 8, 0, 0)) == -300);
    assert(timezone_offset_for_profile_at(*central, utc_epoch(2026, 11, 1, 6, 59, 59)) == -300);
    assert(timezone_offset_for_profile_at(*central, utc_epoch(2026, 11, 1, 7, 0, 0)) == -360);
    assert(timezone_profile_id_for_legacy_offset(-360) == central->id);
    assert(timezone_profile_id_for_legacy_offset(0) == timezone_profile_by_tzdef("UTC0")->id);
    assert(timezone_profile_id_for_legacy_offset(60) == timezone_profile_by_tzdef("CET-1CEST,M3.5.0/2,M10.5.0/3")->id);

    const auto* phoenix = timezone_profile_by_tzdef("MST7");
    assert(phoenix != nullptr);
    assert(timezone_offset_for_profile_at(*phoenix, utc_epoch(2026, 1, 15, 12, 0, 0)) == -420);
    assert(timezone_offset_for_profile_at(*phoenix, utc_epoch(2026, 5, 22, 12, 0, 0)) == -420);
    assert(timezone_profile_id_for_offset(-420) == phoenix->id);
    assert(timezone_profile_id_for_legacy_offset(-420) != phoenix->id);
    assert(timezone_offset_for_profile_id_at(kFixedTimezoneProfileId, -195, utc_epoch(2026, 5, 22, 12, 0, 0)) ==
           -195);

    int parsed = 0;
    assert(parse_posix_tz_standard_offset_minutes("EST5EDT,M3.2.0/2,M11.1.0/2", &parsed));
    assert(parsed == -300);
    assert(parse_posix_tz_standard_offset_minutes("UTC+5:30", &parsed));
    assert(parsed == -330);

    char fixed[24] = {};
    build_fixed_posix_tzdef(-300, fixed, sizeof(fixed));
    assert(std::string(fixed) == "UTC+5");

    return 0;
}
