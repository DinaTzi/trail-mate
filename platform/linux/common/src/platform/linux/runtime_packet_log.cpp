#include "platform/linux/runtime_packet_log.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <deque>
#include <mutex>

namespace platform::linux_runtime
{
namespace
{

constexpr std::size_t kMaxEntriesPerSource = 240;

std::mutex s_mutex;
std::deque<PacketLogEntry> s_gps_entries;
std::deque<PacketLogEntry> s_lora_entries;
std::deque<PacketLogEntry> s_mqtt_entries;

std::deque<PacketLogEntry>& entries_for(PacketLogSource source)
{
    switch (source)
    {
    case PacketLogSource::Lora:
        return s_lora_entries;
    case PacketLogSource::Mqtt:
        return s_mqtt_entries;
    case PacketLogSource::Gps:
    default:
        return s_gps_entries;
    }
}

std::uint64_t now_ms()
{
    using clock = std::chrono::system_clock;
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            clock::now().time_since_epoch())
            .count());
}

} // namespace

void append_packet_log(PacketLogEntry entry)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    if (entry.timestamp_ms == 0)
    {
        entry.timestamp_ms = now_ms();
    }
    auto& entries = entries_for(entry.source);
    entries.push_back(std::move(entry));
    while (entries.size() > kMaxEntriesPerSource)
    {
        entries.pop_front();
    }
}

std::vector<PacketLogEntry> recent_packet_logs(PacketLogSource source,
                                               std::size_t max_entries)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    const auto& entries = entries_for(source);
    const std::size_t count = std::min(max_entries, entries.size());
    std::vector<PacketLogEntry> out{};
    out.reserve(count);
    const auto first = entries.end() - static_cast<std::ptrdiff_t>(count);
    for (auto it = first; it != entries.end(); ++it)
    {
        out.push_back(*it);
    }
    std::reverse(out.begin(), out.end());
    return out;
}

void clear_packet_logs(PacketLogSource source)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    entries_for(source).clear();
}

const char* packet_log_source_label(PacketLogSource source) noexcept
{
    switch (source)
    {
    case PacketLogSource::Lora:
        return "LoRa";
    case PacketLogSource::Mqtt:
        return "MQTT";
    case PacketLogSource::Gps:
    default:
        return "GPS";
    }
}

const char* packet_log_direction_label(PacketLogDirection direction) noexcept
{
    switch (direction)
    {
    case PacketLogDirection::Tx:
        return "TX";
    case PacketLogDirection::System:
        return "SYS";
    case PacketLogDirection::Rx:
    default:
        return "RX";
    }
}

const char* packet_log_segment_class(PacketLogSegmentKind kind) noexcept
{
    switch (kind)
    {
    case PacketLogSegmentKind::Header:
        return "log-segment-header";
    case PacketLogSegmentKind::Body:
        return "log-segment-body";
    case PacketLogSegmentKind::Checksum:
        return "log-segment-checksum";
    case PacketLogSegmentKind::Error:
        return "log-segment-error";
    case PacketLogSegmentKind::Meta:
    default:
        return "log-segment-meta";
    }
}

std::string hex_bytes(const std::uint8_t* data, std::size_t size)
{
    if (data == nullptr || size == 0)
    {
        return {};
    }

    std::string out{};
    out.reserve(size * 3);
    char byte[4] = {};
    for (std::size_t index = 0; index < size; ++index)
    {
        if (index != 0)
        {
            out += ' ';
        }
        std::snprintf(byte, sizeof(byte), "%02X",
                      static_cast<unsigned>(data[index]));
        out += byte;
    }
    return out;
}

std::string hex_bytes(const char* data, std::size_t size)
{
    return hex_bytes(reinterpret_cast<const std::uint8_t*>(data), size);
}

} // namespace platform::linux_runtime
