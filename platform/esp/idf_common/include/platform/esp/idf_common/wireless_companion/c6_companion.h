#pragma once

#include <cstdint>

namespace platform::esp::idf_common::wireless_companion
{

enum class BleBackend : uint8_t
{
    None = 0,
    Local = 1,
    C6Companion = 2,
};

enum class CompanionState : uint8_t
{
    Unsupported = 0,
    NotStarted = 1,
    Missing = 2,
    TransportPending = 3,
    Present = 4,
    Error = 5,
};

struct C6CompanionStatus
{
    bool board_capable = false;
    bool started = false;
    bool present = false;
    CompanionState state = CompanionState::Unsupported;
    uint16_t protocol_min = 1;
    uint16_t protocol_max = 1;
    uint16_t selected_protocol = 0;
    uint32_t supported_features = 0;
    uint32_t firmware_version = 0;
    uint32_t free_heap = 0;
    uint32_t enabled_features = 0;
    uint32_t config_seq = 0;
    uint16_t config_error = 0;
    uint16_t selected_mtu = 0;
    uint8_t ble_state = 0;
    uint8_t espnow_state = 0;
    uint8_t wifi_state = 0;
    uint32_t ping_nonce = 0;
    uint32_t ping_count = 0;
    uint32_t pong_count = 0;
    const char* detail = "unsupported";
};

class WirelessCompanion
{
  public:
    virtual bool begin() = 0;
    virtual bool isPresent() const = 0;
    virtual uint32_t capabilities() const = 0;
    virtual C6CompanionStatus status() const = 0;
    virtual void poll() = 0;
    virtual ~WirelessCompanion() = default;
};

WirelessCompanion& c6_companion();
bool ensure_c6_companion_started();
C6CompanionStatus get_c6_companion_status();
const char* companion_state_name(CompanionState state);

} // namespace platform::esp::idf_common::wireless_companion
