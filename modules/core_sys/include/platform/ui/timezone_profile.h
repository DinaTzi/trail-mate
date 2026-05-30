#pragma once

#include <cstddef>
#include <cstdint>
#include <ctime>

namespace platform::ui::time
{

enum class DstRuleKind : uint8_t
{
    None,
    Us,
    Eu,
    Australia,
    NewZealand,
};

struct TimezoneProfile
{
    constexpr TimezoneProfile(int profile_id = 0,
                              const char* profile_label = "",
                              const char* profile_tzdef = "",
                              int standard_offset = 0,
                              int daylight_offset = 0,
                              DstRuleKind rule = DstRuleKind::None)
        : id(profile_id),
          label(profile_label),
          tzdef(profile_tzdef),
          standard_offset_min(standard_offset),
          daylight_offset_min(daylight_offset),
          dst_rule(rule)
    {
    }

    int id;
    const char* label;
    const char* tzdef;
    int standard_offset_min;
    int daylight_offset_min;
    DstRuleKind dst_rule;
};

constexpr int kFixedTimezoneProfileId = -1;

const TimezoneProfile* timezone_profiles(std::size_t* out_count);
const TimezoneProfile* timezone_profile_by_id(int id);
const TimezoneProfile* timezone_profile_by_tzdef(const char* tzdef);
const TimezoneProfile* timezone_profile_by_offset(int offset_min);
const TimezoneProfile* default_timezone_profile();
bool timezone_profile_id_is_fixed(int profile_id);
int timezone_offset_for_profile_at(const TimezoneProfile& profile, std::time_t utc_seconds);
int timezone_offset_for_profile_id_at(int profile_id, std::time_t utc_seconds);
int timezone_offset_for_profile_id_at(int profile_id, int fixed_offset_min, std::time_t utc_seconds);
int timezone_profile_id_for_offset(int offset_min);
int timezone_profile_id_for_legacy_offset(int offset_min);
int timezone_profile_id_for_fixed_offset(int offset_min);
bool parse_posix_tz_standard_offset_minutes(const char* tzdef, int* out_offset_min);
void build_fixed_posix_tzdef(int offset_min, char* out, std::size_t out_len);

} // namespace platform::ui::time
