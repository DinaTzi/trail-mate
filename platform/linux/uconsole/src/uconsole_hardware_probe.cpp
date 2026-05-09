#include "uconsole/uconsole_hardware_probe.h"

#include <algorithm>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

namespace trailmate::uconsole
{
namespace
{

namespace fs = std::filesystem;

[[nodiscard]] bool containsToken(const std::string& text,
                                 const char* token) noexcept
{
    return text.find(token) != std::string::npos;
}

[[nodiscard]] std::string pathString(const fs::path& path)
{
    return path.string();
}

[[nodiscard]] bool existingPath(const fs::path& path)
{
    std::error_code ec;
    return fs::exists(path, ec) && !ec;
}

[[nodiscard]] std::vector<fs::path> directoryEntries(const fs::path& dir)
{
    std::vector<fs::path> out{};
    std::error_code ec;
    if (!fs::exists(dir, ec) || ec)
    {
        return out;
    }

    for (const auto& entry : fs::directory_iterator(dir, ec))
    {
        if (ec)
        {
            break;
        }
        out.push_back(entry.path());
    }
    std::sort(out.begin(), out.end());
    return out;
}

[[nodiscard]] fs::path resolvedDevicePath(const fs::path& path)
{
    std::error_code ec;
    fs::path resolved = fs::canonical(path, ec);
    if (ec)
    {
        return path;
    }
    return resolved;
}

[[nodiscard]] bool findClockworkPiSerial(fs::path& out_path)
{
    for (const auto& path : directoryEntries("/dev/serial/by-id"))
    {
        const std::string name = path.filename().string();
        if (containsToken(name, "ClockworkPI") &&
            containsToken(name, "uConsole"))
        {
            out_path = path;
            return true;
        }
    }

    return false;
}

[[nodiscard]] bool findSpiDevice(fs::path& out_path)
{
    if (existingPath("/dev/spidev4.0"))
    {
        out_path = "/dev/spidev4.0";
        return true;
    }

    for (const auto& path : directoryEntries("/dev"))
    {
        const std::string name = path.filename().string();
        if (name.rfind("spidev", 0) == 0)
        {
            out_path = path;
            return true;
        }
    }
    return false;
}

[[nodiscard]] std::string summarizeI2c()
{
    std::vector<std::string> names{};
    for (const auto& path : directoryEntries("/dev"))
    {
        const std::string name = path.filename().string();
        if (name.rfind("i2c-", 0) == 0)
        {
            names.push_back("/dev/" + name);
        }
    }

    if (names.empty())
    {
        return {};
    }

    std::ostringstream out;
    for (std::size_t index = 0; index < names.size(); ++index)
    {
        if (index != 0)
        {
            out << ", ";
        }
        out << names[index];
    }
    return out.str();
}

} // namespace

bool uconsoleAutoGpsSerialPath(std::string& out_path)
{
    fs::path path{};
    if (!findClockworkPiSerial(path))
    {
        out_path.clear();
        return false;
    }
    out_path = pathString(path);
    return true;
}

UConsoleHardwareProbe probeUConsoleHardware()
{
    UConsoleHardwareProbe out{};

    fs::path serial_path{};
    if (findClockworkPiSerial(serial_path))
    {
        out.aio2_detected = true;
        out.gps_serial_detected = true;
        out.aio2_serial_path = pathString(resolvedDevicePath(serial_path));
        out.gps_serial_path = pathString(serial_path);
    }

    fs::path spi_path{};
    if (findSpiDevice(spi_path))
    {
        out.aio2_detected = true;
        out.lora_spi_detected = true;
        out.lora_spi_path = pathString(spi_path);
    }

    out.i2c_summary = summarizeI2c();
    out.i2c_detected = !out.i2c_summary.empty();

    std::ostringstream summary;
    summary << (out.aio2_detected ? "AIO2 endpoints present"
                                  : "No AIO2 endpoint detected");
    if (!out.aio2_serial_path.empty())
    {
        summary << " / serial " << out.aio2_serial_path;
    }
    if (!out.lora_spi_path.empty())
    {
        summary << " / SPI " << out.lora_spi_path;
    }
    if (out.i2c_detected)
    {
        summary << " / I2C " << out.i2c_summary;
    }
    out.summary = summary.str();
    return out;
}

} // namespace trailmate::uconsole
