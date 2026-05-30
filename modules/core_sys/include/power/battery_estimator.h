#pragma once

#include <cstdint>

namespace power
{

struct BatteryEstimatorSample
{
    uint32_t now_ms = 0;
    int pmu_percent = -1;
    int pmu_battery_mv = -1;
    int adc_battery_mv = -1;
    bool charging = false;
    bool vbus_present = false;
};

enum class BatteryEstimateReason
{
    NoSample,
    PmuPercent,
    VoltageFallback,
    ChargerStableHold,
    DetachStableHold,
    DropGuard,
    ZeroGuard,
    RateLimited,
};

struct BatteryEstimate
{
    BatteryEstimate() = default;
    BatteryEstimate(int percent_value, BatteryEstimateReason reason_value)
        : percent(percent_value), reason(reason_value)
    {
    }

    int percent = -1;
    BatteryEstimateReason reason = BatteryEstimateReason::NoSample;
};

class BatteryEstimator
{
  public:
    BatteryEstimate update(const BatteryEstimatorSample& sample);
    int lastPercent() const { return last_percent_; }
    uint32_t lastVbusDetachMs() const { return last_vbus_detach_ms_; }
    void reset();

  private:
    int last_percent_ = -1;
    uint32_t last_sample_ms_ = 0;
    uint32_t last_vbus_detach_ms_ = 0;
    bool has_last_power_state_ = false;
    bool last_vbus_present_ = false;
    bool last_charging_ = false;
    uint8_t zero_streak_ = 0;
};

int batteryPercentFromLipoMillivolts(int mv);

} // namespace power
