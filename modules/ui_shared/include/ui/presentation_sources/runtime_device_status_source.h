#pragma once

#include "ui_presentation/device/device_status_source.h"

namespace ui::presentation_sources
{

class RuntimeDeviceStatusSource final : public ui::device::IDeviceStatusSource
{
  public:
    bool buildDeviceStatusSnapshot(ui::device::DeviceStatusSnapshot& out) const override;
};

RuntimeDeviceStatusSource& runtime_device_status_source();

} // namespace ui::presentation_sources
