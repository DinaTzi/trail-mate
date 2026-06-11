#include "ui/presentation_sources/legacy_air_device_status_source.h"

#include "ui/presentation_sources/runtime_device_status_source.h"

namespace ui::presentation_sources
{

bool LegacyAirDeviceStatusSource::buildDeviceStatusSnapshot(ui::device::DeviceStatusSnapshot& out) const
{
    return runtime_device_status_source().buildDeviceStatusSnapshot(out);
}

LegacyAirDeviceStatusSource& legacy_air_device_status_source()
{
    static LegacyAirDeviceStatusSource source;
    return source;
}

} // namespace ui::presentation_sources
