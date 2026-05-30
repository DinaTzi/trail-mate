#include "platform/ui/time_runtime.h"

#include "platform/ui/settings_store.h"
#include "platform/ui/timezone_profile.h"

#include <cstdlib>
#include <ctime>

namespace platform::ui::time
{
namespace
{

constexpr const char* kSettingsNs = "settings";
constexpr const char* kTimezoneOffsetKey = "timezone_offset";
constexpr const char* kTimezoneProfileKey = "timezone_profile";
constexpr const char* kTimezoneOffsetEnv = "TRAIL_MATE_TZ_OFFSET_MIN";

int env_timezone_offset_or_default(int fallback)
{
    const char* value = std::getenv(kTimezoneOffsetEnv);
    if (!value || value[0] == '\0')
    {
        return fallback;
    }

    char* end = nullptr;
    const long parsed = std::strtol(value, &end, 10);
    if (end == value || (end && *end != '\0'))
    {
        return fallback;
    }
    return static_cast<int>(parsed);
}

} // namespace

int timezone_offset_min()
{
    const int fixed_offset_min = env_timezone_offset_or_default(
        ::platform::ui::settings_store::get_int(kSettingsNs, kTimezoneOffsetKey, 0));
    return timezone_offset_for_profile_id_at(timezone_profile_id(), fixed_offset_min, ::time(nullptr));
}

void set_timezone_offset_min(int offset_min)
{
    ::platform::ui::settings_store::put_int(kSettingsNs, kTimezoneOffsetKey, offset_min);
    ::platform::ui::settings_store::put_int(kSettingsNs,
                                            kTimezoneProfileKey,
                                            timezone_profile_id_for_fixed_offset(offset_min));
}

int timezone_profile_id()
{
    const int offset = env_timezone_offset_or_default(
        ::platform::ui::settings_store::get_int(kSettingsNs, kTimezoneOffsetKey, 0));
    const int profile_id = ::platform::ui::settings_store::get_int(
        kSettingsNs,
        kTimezoneProfileKey,
        timezone_profile_id_for_legacy_offset(offset));
    return timezone_profile_id_is_fixed(profile_id) || timezone_profile_by_id(profile_id)
               ? profile_id
               : timezone_profile_id_for_legacy_offset(offset);
}

void set_timezone_profile_id(int profile_id)
{
    const TimezoneProfile* profile = timezone_profile_by_id(profile_id);
    if (!profile)
    {
        profile = default_timezone_profile();
    }
    ::platform::ui::settings_store::put_int(kSettingsNs, kTimezoneProfileKey, profile->id);
    ::platform::ui::settings_store::put_int(kSettingsNs, kTimezoneOffsetKey, profile->standard_offset_min);
}

::time_t apply_timezone_offset(::time_t utc_seconds)
{
    const int fixed_offset_min = env_timezone_offset_or_default(
        ::platform::ui::settings_store::get_int(kSettingsNs, kTimezoneOffsetKey, 0));
    return utc_seconds +
           static_cast<time_t>(timezone_offset_for_profile_id_at(timezone_profile_id(), fixed_offset_min, utc_seconds)) *
               60;
}

::time_t apply_timezone_offset_for_utc(::time_t utc_seconds)
{
    return apply_timezone_offset(utc_seconds);
}

bool localtime_now(struct tm* out_tm)
{
    if (!out_tm)
    {
        return false;
    }

    const ::time_t now = ::time(nullptr);
    if (now <= 0)
    {
        return false;
    }

    const ::time_t local = apply_timezone_offset(now);
    const ::tm* tmp = ::gmtime(&local);
    if (!tmp)
    {
        return false;
    }

    *out_tm = *tmp;
    return true;
}

} // namespace platform::ui::time
