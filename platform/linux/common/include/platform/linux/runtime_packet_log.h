#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace platform::linux_runtime
{

enum class PacketLogSource
{
    Gps,
    Lora,
};

enum class PacketLogDirection
{
    Rx,
    Tx,
    System,
};

enum class PacketLogSegmentKind
{
    Header,
    Body,
    Checksum,
    Meta,
    Error,
};

struct PacketLogSegment
{
    PacketLogSegmentKind kind = PacketLogSegmentKind::Meta;
    std::string label{};
    std::string text{};
};

struct PacketLogEntry
{
    PacketLogSource source = PacketLogSource::Gps;
    PacketLogDirection direction = PacketLogDirection::Rx;
    std::uint64_t timestamp_ms = 0;
    std::string title{};
    std::string summary{};
    std::string raw_hex{};
    std::vector<PacketLogSegment> segments{};
};

void append_packet_log(PacketLogEntry entry);
std::vector<PacketLogEntry> recent_packet_logs(PacketLogSource source,
                                               std::size_t max_entries);
void clear_packet_logs(PacketLogSource source);

const char* packet_log_source_label(PacketLogSource source) noexcept;
const char* packet_log_direction_label(PacketLogDirection direction) noexcept;
const char* packet_log_segment_class(PacketLogSegmentKind kind) noexcept;
std::string hex_bytes(const std::uint8_t* data, std::size_t size);
std::string hex_bytes(const char* data, std::size_t size);

} // namespace platform::linux_runtime
