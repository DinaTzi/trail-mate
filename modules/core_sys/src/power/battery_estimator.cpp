#include "power/battery_estimator.h"

#include <cstddef>

namespace power
{
namespace
{

constexpr uint32_t kMinReadIntervalMs = 1000;
constexpr uint32_t kDetachStabilizeMs = 120000;
constexpr int kDropGuardPct = 12;
constexpr int kChargeDropGuardPct = 3;
constexpr int kAllowedUnplugDropPct = 5;
constexpr int kVoltageSupportMarginPct = 10;
constexpr int kRiseGuardPct = 2;
constexpr int kZeroGuardMinPct = 15;
constexpr uint8_t kZeroGuardCount = 3;

int clamp_percent(int percent)
{
    if (percent < 0)
    {
        return -1;
    }
    if (percent > 100)
    {
        return 100;
    }
    return percent;
}

int best_voltage_percent(const BatteryEstimatorSample& sample)
{
    int percent = batteryPercentFromLipoMillivolts(sample.pmu_battery_mv);
    if (percent < 0)
    {
        percent = batteryPercentFromLipoMillivolts(sample.adc_battery_mv);
    }
    return percent;
}

bool voltage_supports_last(int voltage_percent, int last_percent)
{
    return voltage_percent >= 0 && last_percent >= 0 &&
           voltage_percent + kVoltageSupportMarginPct >= last_percent;
}

} // namespace

int batteryPercentFromLipoMillivolts(int mv)
{
    if (mv <= 0)
    {
        return -1;
    }

    static constexpr int kCurve[][2] = {
        {4200, 100},
        {4100, 90},
        {4000, 80},
        {3900, 60},
        {3800, 40},
        {3700, 20},
        {3600, 10},
        {3300, 0},
    };

    if (mv >= kCurve[0][0])
    {
        return 100;
    }
    if (mv <= kCurve[7][0])
    {
        return 0;
    }

    for (std::size_t i = 0; i + 1 < (sizeof(kCurve) / sizeof(kCurve[0])); ++i)
    {
        const int v1 = kCurve[i][0];
        const int p1 = kCurve[i][1];
        const int v2 = kCurve[i + 1][0];
        const int p2 = kCurve[i + 1][1];
        if (mv <= v1 && mv >= v2)
        {
            return p2 + (mv - v2) * (p1 - p2) / (v1 - v2);
        }
    }

    return -1;
}

BatteryEstimate BatteryEstimator::update(const BatteryEstimatorSample& sample)
{
    const bool was_powered = has_last_power_state_ && (last_vbus_present_ || last_charging_);
    const bool is_powered = sample.vbus_present || sample.charging;
    if (was_powered && !is_powered)
    {
        last_vbus_detach_ms_ = sample.now_ms;
    }

    has_last_power_state_ = true;
    last_vbus_present_ = sample.vbus_present;
    last_charging_ = sample.charging;

    if (last_percent_ >= 0 && last_sample_ms_ != 0 &&
        sample.now_ms - last_sample_ms_ < kMinReadIntervalMs)
    {
        return {last_percent_, BatteryEstimateReason::RateLimited};
    }

    const int voltage_percent = best_voltage_percent(sample);
    int level = clamp_percent(sample.pmu_percent);
    BatteryEstimateReason reason = BatteryEstimateReason::PmuPercent;
    if (level < 0)
    {
        level = voltage_percent;
        reason = BatteryEstimateReason::VoltageFallback;
    }

    if (level < 0)
    {
        last_sample_ms_ = sample.now_ms;
        return {last_percent_, BatteryEstimateReason::NoSample};
    }

    if (last_percent_ >= 0)
    {
        if (!sample.charging && level > last_percent_ + kRiseGuardPct)
        {
            level = last_percent_;
            reason = BatteryEstimateReason::DropGuard;
        }

        if (sample.charging && level + kChargeDropGuardPct < last_percent_)
        {
            level = last_percent_;
            reason = BatteryEstimateReason::ChargerStableHold;
        }

        const bool in_detach_window =
            last_vbus_detach_ms_ != 0 && sample.now_ms - last_vbus_detach_ms_ <= kDetachStabilizeMs;
        if (!is_powered && in_detach_window && level + kAllowedUnplugDropPct < last_percent_ &&
            voltage_supports_last(voltage_percent, last_percent_))
        {
            level = last_percent_;
            reason = BatteryEstimateReason::DetachStableHold;
        }

        if (!sample.charging && level + kDropGuardPct < last_percent_ &&
            voltage_supports_last(voltage_percent, last_percent_))
        {
            level = last_percent_;
            reason = BatteryEstimateReason::DropGuard;
        }
    }

    if (!sample.charging && level == 0 && last_percent_ >= kZeroGuardMinPct)
    {
        if (zero_streak_ < kZeroGuardCount)
        {
            ++zero_streak_;
            last_sample_ms_ = sample.now_ms;
            return {last_percent_, BatteryEstimateReason::ZeroGuard};
        }
    }
    else
    {
        zero_streak_ = 0;
    }

    last_percent_ = level;
    last_sample_ms_ = sample.now_ms;
    return {last_percent_, reason};
}

void BatteryEstimator::reset()
{
    last_percent_ = -1;
    last_sample_ms_ = 0;
    last_vbus_detach_ms_ = 0;
    has_last_power_state_ = false;
    last_vbus_present_ = false;
    last_charging_ = false;
    zero_streak_ = 0;
}

} // namespace power
