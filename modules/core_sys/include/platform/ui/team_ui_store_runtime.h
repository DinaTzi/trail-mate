/**
 * @file team_ui_store_runtime.h
 * @brief Platform-facing Team UI persistence contract consumed by shared UI
 */

#pragma once

#include "platform/ui/team_ui_chat_log_store.h"
#include "platform/ui/team_ui_snapshot_store.h"
#include "team/domain/team_types.h"
#include "team/protocol/team_chat.h"
#include "team/protocol/team_mgmt.h"
#include "team/protocol/team_track.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace team
{
namespace ui
{

enum class TeamKeyEventType : uint8_t
{
    TeamCreated = 1,
    MemberAccepted = 2,
    MemberKicked = 3,
    LeaderTransferred = 4,
    EpochRotated = 5
};

struct TeamPosSample
{
    uint32_t member_id = 0;
    int32_t lat_e7 = 0;
    int32_t lon_e7 = 0;
    int16_t alt_m = 0;
    uint16_t speed_dmps = 0;
    uint32_t ts = 0;
};

using ITeamUiStore = ITeamUiSnapshotStore;
using TeamUiMemoryStore = TeamUiSnapshotMemoryStore;

ITeamUiStore& team_ui_get_store();
void team_ui_set_store(ITeamUiStore* store);
bool team_ui_append_key_event(const TeamId& team_id,
                              TeamKeyEventType type,
                              uint32_t event_seq,
                              uint32_t ts,
                              const uint8_t* payload,
                              size_t len);
bool team_ui_posring_append(const TeamId& team_id,
                            uint32_t member_id,
                            int32_t lat_e7,
                            int32_t lon_e7,
                            int16_t alt_m,
                            uint16_t speed_dmps,
                            uint32_t ts);
bool team_ui_posring_load_latest(const TeamId& team_id,
                                 std::vector<TeamPosSample>& out);
bool team_ui_chatlog_append(const TeamId& team_id,
                            uint32_t peer_id,
                            bool incoming,
                            uint32_t ts,
                            const std::string& text);
bool team_ui_chatlog_append_structured(const TeamId& team_id,
                                       uint32_t peer_id,
                                       bool incoming,
                                       uint32_t ts,
                                       team::proto::TeamChatType type,
                                       const std::vector<uint8_t>& payload);
bool team_ui_chatlog_load_recent(const TeamId& team_id,
                                 size_t max_count,
                                 std::vector<TeamChatLogEntry>& out);
bool team_ui_save_keys_now(const TeamId& team_id,
                           uint32_t key_id,
                           const std::array<uint8_t, team::proto::kTeamChannelPskSize>& psk);
bool team_ui_append_member_track(const TeamId& team_id,
                                 uint32_t member_id,
                                 const team::proto::TeamTrackMessage& track);
bool team_ui_get_member_track_path(const TeamId& team_id,
                                   uint32_t member_id,
                                   std::string& out_path);

} // namespace ui
} // namespace team
