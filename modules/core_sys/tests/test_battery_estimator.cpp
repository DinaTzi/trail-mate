#include "power/battery_estimator.h"

#include <cassert>

namespace
{

power::BatteryEstimatorSample sample(uint32_t now_ms,
                                     int pmu_percent,
                                     int pmu_battery_mv,
                                     int adc_battery_mv,
                                     bool charging,
                                     bool vbus_present)
{
    power::BatteryEstimatorSample s{};
    s.now_ms = now_ms;
    s.pmu_percent = pmu_percent;
    s.pmu_battery_mv = pmu_battery_mv;
    s.adc_battery_mv = adc_battery_mv;
    s.charging = charging;
    s.vbus_present = vbus_present;
    return s;
}

void holds_full_reading_during_unplug_stabilization()
{
    power::BatteryEstimator estimator;
    auto result = estimator.update(sample(1000, 100, 4200, 4190, true, true));
    assert(result.percent == 100);

    result = estimator.update(sample(3000, 82, 4120, 4110, false, false));
    assert(result.percent == 100);
    assert(result.reason == power::BatteryEstimateReason::DetachStableHold);
}

void accepts_sustained_low_voltage_after_detach_window()
{
    power::BatteryEstimator estimator;
    auto result = estimator.update(sample(1000, 100, 4200, 4190, true, true));
    assert(result.percent == 100);

    result = estimator.update(sample(130000, 72, 3950, 3940, false, false));
    assert(result.percent == 72);
}

void uses_voltage_curve_when_pmu_percent_is_invalid()
{
    power::BatteryEstimator estimator;
    auto result = estimator.update(sample(1000, -1, 3900, -1, false, false));
    assert(result.percent == 60);
    assert(result.reason == power::BatteryEstimateReason::VoltageFallback);
}

void suppresses_small_unplugged_rises()
{
    power::BatteryEstimator estimator;
    auto result = estimator.update(sample(1000, 74, 3970, 3970, false, false));
    assert(result.percent == 74);

    result = estimator.update(sample(3000, 79, 3980, 3980, false, false));
    assert(result.percent == 74);
    assert(result.reason == power::BatteryEstimateReason::DropGuard);
}

void guards_repeated_fake_zero_samples()
{
    power::BatteryEstimator estimator;
    auto result = estimator.update(sample(1000, 45, 3820, 3820, false, false));
    assert(result.percent == 45);

    result = estimator.update(sample(3000, 0, -1, -1, false, false));
    assert(result.percent == 45);
    assert(result.reason == power::BatteryEstimateReason::ZeroGuard);
}

} // namespace

int main()
{
    assert(power::batteryPercentFromLipoMillivolts(4200) == 100);
    assert(power::batteryPercentFromLipoMillivolts(4000) == 80);
    assert(power::batteryPercentFromLipoMillivolts(3900) == 60);
    assert(power::batteryPercentFromLipoMillivolts(3300) == 0);

    holds_full_reading_during_unplug_stabilization();
    accepts_sustained_low_voltage_after_detach_window();
    uses_voltage_curve_when_pmu_percent_is_invalid();
    suppresses_small_unplugged_rises();
    guards_repeated_fake_zero_samples();
    return 0;
}
