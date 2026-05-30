#pragma once

#include "platform/ui/team_ui_store_runtime.h"

#include <cstdint>

namespace team
{
namespace ui
{

struct TeamPageKeyEventState
{
    TeamId team_id{};
    bool has_team_id = false;
    uint32_t last_event_seq = 0;
    uint32_t security_round = 0;
};

class ITeamPageKeyEventWriter
{
  public:
    virtual ~ITeamPageKeyEventWriter() = default;
    virtual bool appendKeyEvent(const TeamId& team_id,
                                TeamKeyEventType type,
                                uint32_t event_seq,
                                uint32_t timestamp_s,
                                const uint8_t* payload,
                                size_t payload_size) = 0;
};

class TeamPageKeyEventLog
{
  public:
    TeamPageKeyEventLog(ITeamPageKeyEventWriter& writer,
                        uint32_t timestamp_s);

    bool appendTeamCreated(TeamPageKeyEventState& state,
                           uint32_t leader_id) const;
    bool appendMemberAccepted(TeamPageKeyEventState& state,
                              uint32_t member_id,
                              uint8_t role) const;
    bool appendMemberKicked(TeamPageKeyEventState& state,
                            uint32_t member_id) const;
    bool appendLeaderTransferred(TeamPageKeyEventState& state,
                                 uint32_t leader_id) const;
    bool appendEpochRotated(TeamPageKeyEventState& state,
                            uint32_t key_id) const;

    static uint64_t teamIdToU64(const TeamId& id);

  private:
    bool append(TeamPageKeyEventState& state,
                TeamKeyEventType type,
                const uint8_t* payload,
                size_t payload_size) const;

    ITeamPageKeyEventWriter& writer_;
    uint32_t timestamp_s_ = 0;
};

} // namespace ui
} // namespace team
