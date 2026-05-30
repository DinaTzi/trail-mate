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

struct TeamPageCommandState
{
    bool in_team = false;
    bool pending_join = false;
    uint32_t pending_join_started_s = 0;
    bool kicked_out = false;
    bool self_is_leader = false;
    uint32_t last_event_seq = 0;
    TeamPairingRole pairing_role = TeamPairingRole::None;
    TeamPairingState pairing_state = TeamPairingState::Idle;
    uint32_t pairing_peer_id = 0;
    std::string pairing_team_name;

    TeamId team_id{};
    bool has_team_id = false;
    std::string team_name;
    uint32_t security_round = 0;
    uint32_t last_update_s = 0;
    std::array<uint8_t, team::proto::kTeamChannelPskSize> team_psk{};
    bool has_team_psk = false;
    bool waiting_new_keys = false;

    int selected_member_index = -1;
    std::vector<TeamMemberUi> members;
};

struct TeamPageCommandContext
{
    uint32_t now_s = 0;
    uint32_t self_node_id = 0;
};

struct TeamPageGeneratedTeamSecrets
{
    TeamId team_id{};
    std::array<uint8_t, team::proto::kTeamChannelPskSize> team_psk{};
};

struct TeamPageKickRotation
{
    bool rotate_keys = false;
    uint32_t key_id = 0;
    std::array<uint8_t, team::proto::kTeamChannelPskSize> team_psk{};
};

struct TeamPageCommandEffects
{
    bool accepted = false;
    bool changed = false;
    bool clear_keys = false;
    bool stop_pairing = false;
    bool reset_controller_ui = false;
    bool clear_keydist_pending = false;
    bool clear_status_pending = false;
    bool team_created_key_event = false;
    bool keys_changed = false;
    bool status_should_send = false;
    bool member_kicked = false;
    uint32_t member_kicked_id = 0;
    bool leader_transferred_key_event = false;
    uint32_t leader_target = 0;
};

class TeamPageCommandReducer
{
  public:
    explicit TeamPageCommandReducer(TeamPageCommandContext context);

    TeamPageCommandEffects reduceCreate(
        TeamPageCommandState& state,
        const TeamPageGeneratedTeamSecrets& generated) const;
    TeamPageCommandEffects reduceLeave(TeamPageCommandState& state) const;
    TeamPageCommandEffects reduceKickedOut(TeamPageCommandState& state) const;
    TeamPageCommandEffects reduceClearKickedOut(
        TeamPageCommandState& state) const;
    TeamPageCommandEffects reducePairingStarted(
        TeamPageCommandState& state,
        TeamPairingRole role,
        TeamPairingState pairing_state) const;
    TeamPageCommandEffects reduceJoinCanceled(
        TeamPageCommandState& state) const;
    TeamPageCommandEffects reduceKickConfirmed(
        TeamPageCommandState& state,
        uint32_t target_node_id,
        const TeamPageKickRotation& rotation) const;
    TeamPageCommandEffects reduceTransferLeader(
        TeamPageCommandState& state,
        uint32_t target_node_id) const;

    static std::string formatTeamNameFromId(const TeamId& id);

  private:
    void resetTeamMembership(TeamPageCommandState& state,
                             bool kicked_out) const;
    void resetPairing(TeamPageCommandState& state) const;
    void ensureSelfMember(TeamPageCommandState& state,
                          bool leader) const;
    int findMemberIndex(const TeamPageCommandState& state,
                        uint32_t node_id) const;
    void assignMemberColor(TeamMemberUi& member) const;

    TeamPageCommandContext context_;
};

} // namespace ui
} // namespace team
