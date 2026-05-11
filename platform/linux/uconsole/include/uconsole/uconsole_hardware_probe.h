#pragma once

#include <string>

namespace trailmate::uconsole
{

struct UConsoleHardwareProbe
{
    bool aio2_detected = false;
    bool gps_serial_detected = false;
    bool lora_spi_detected = false;
    bool i2c_detected = false;

    std::string aio2_serial_path{};
    std::string gps_serial_path{};
    std::string lora_spi_path{};
    std::string i2c_summary{};
    std::string summary{};
};

[[nodiscard]] UConsoleHardwareProbe probeUConsoleHardware();
[[nodiscard]] bool uconsoleAutoGpsSerialPath(std::string& out_path);

} // namespace trailmate::uconsole
