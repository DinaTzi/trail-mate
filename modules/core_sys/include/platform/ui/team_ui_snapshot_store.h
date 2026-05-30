/**
 * @file team_ui_snapshot_store.h
 * @brief Narrow Team UI snapshot persistence contract.
 */

#pragma once

#include "platform/ui/team_ui_types.h"
#include "team/domain/team_types.h"
#include "team/protocol/team_mgmt.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace team
{
namespace ui
{

struct TeamUiSnapshot
{
    bool in_team = false;
    bool pending_join = false;
    uint32_t pending_join_started_s = 0;
    bool kicked_out = false;
    bool self_is_leader = false;
    uint32_t last_event_seq = 0;
    uint32_t team_chat_unread = 0;

    TeamId team_id{};
    bool has_team_id = false;

    std::string team_name;
    uint32_t security_round = 0;
    uint32_t last_update_s = 0;
    std::array<uint8_t, team::proto::kTeamChannelPskSize> team_psk{};
    bool has_team_psk = false;

    std::vector<TeamMemberUi> members;
};

class ITeamUiSnapshotStore
{
  public:
    virtual ~ITeamUiSnapshotStore() = default;
    virtual bool load(TeamUiSnapshot& out) = 0;
    virtual void save(const TeamUiSnapshot& in) = 0;
    virtual void clear() = 0;
};

// Simple in-memory fallback store used when no platform persistence store is installed.
class TeamUiSnapshotMemoryStore : public ITeamUiSnapshotStore
{
  public:
    bool load(TeamUiSnapshot& out) override;
    void save(const TeamUiSnapshot& in) override;
    void clear() override;

  private:
    static bool has_snapshot_;
    static TeamUiSnapshot snapshot_;
};

ITeamUiSnapshotStore& team_ui_snapshot_store();
void team_ui_set_snapshot_store(ITeamUiSnapshotStore* store);

} // namespace ui
} // namespace team
