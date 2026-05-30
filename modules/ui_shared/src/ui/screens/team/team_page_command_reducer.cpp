#include "ui/screens/team/team_page_command_reducer.h"

#include <algorithm>
#include <cstdio>

namespace team
{
namespace ui
{

TeamPageCommandReducer::TeamPageCommandReducer(
    TeamPageCommandContext context)
    : context_(context)
{
}

TeamPageCommandEffects TeamPageCommandReducer::reduceCreate(
    TeamPageCommandState& state,
    const TeamPageGeneratedTeamSecrets& generated) const
{
    TeamPageCommandEffects effects;
    state.in_team = true;
    state.pending_join = false;
    state.pending_join_started_s = 0;
    state.kicked_out = false;
    state.self_is_leader = true;
    state.members.clear();

    if (!state.has_team_id)
    {
        state.team_id = generated.team_id;
        state.has_team_id = true;
        state.team_name = formatTeamNameFromId(state.team_id);
    }
    if (state.security_round == 0)
    {
        state.security_round = 1;
    }
    if (!state.has_team_psk)
    {
        state.team_psk = generated.team_psk;
        state.has_team_psk = true;
    }

    ensureSelfMember(state, true);
    state.last_update_s = context_.now_s;

    effects.accepted = true;
    effects.changed = true;
    effects.team_created_key_event = state.has_team_id;
    effects.keys_changed = state.has_team_id && state.has_team_psk;
    return effects;
}

TeamPageCommandEffects TeamPageCommandReducer::reduceLeave(
    TeamPageCommandState& state) const
{
    TeamPageCommandEffects effects;
    resetTeamMembership(state, false);
    effects.accepted = true;
    effects.changed = true;
    effects.clear_keys = true;
    effects.stop_pairing = true;
    effects.clear_keydist_pending = true;
    effects.clear_status_pending = true;
    return effects;
}

TeamPageCommandEffects TeamPageCommandReducer::reduceKickedOut(
    TeamPageCommandState& state) const
{
    TeamPageCommandEffects effects;
    resetTeamMembership(state, true);
    effects.accepted = true;
    effects.changed = true;
    effects.clear_keys = true;
    effects.stop_pairing = true;
    effects.clear_keydist_pending = true;
    effects.clear_status_pending = true;
    return effects;
}

TeamPageCommandEffects TeamPageCommandReducer::reduceClearKickedOut(
    TeamPageCommandState& state) const
{
    TeamPageCommandEffects effects;
    state.kicked_out = false;
    effects.accepted = true;
    effects.changed = true;
    return effects;
}

TeamPageCommandEffects TeamPageCommandReducer::reducePairingStarted(
    TeamPageCommandState& state,
    TeamPairingRole role,
    TeamPairingState pairing_state) const
{
    TeamPageCommandEffects effects;
    state.pending_join = true;
    state.pending_join_started_s = context_.now_s;
    state.pairing_role = role;
    state.pairing_state = pairing_state;
    effects.accepted = true;
    effects.changed = true;
    return effects;
}

TeamPageCommandEffects TeamPageCommandReducer::reduceJoinCanceled(
    TeamPageCommandState& state) const
{
    TeamPageCommandEffects effects;
    state.pending_join = false;
    state.pending_join_started_s = 0;
    resetPairing(state);
    effects.accepted = true;
    effects.changed = true;
    effects.reset_controller_ui = true;
    effects.stop_pairing = true;
    return effects;
}

TeamPageCommandEffects TeamPageCommandReducer::reduceKickConfirmed(
    TeamPageCommandState& state,
    uint32_t target_node_id,
    const TeamPageKickRotation& rotation) const
{
    TeamPageCommandEffects effects;
    const int index = findMemberIndex(state, target_node_id);
    if (index < 0)
    {
        return effects;
    }

    if (rotation.rotate_keys)
    {
        state.security_round = rotation.key_id;
        state.team_psk = rotation.team_psk;
        state.has_team_psk = true;
        state.waiting_new_keys = false;
        effects.keys_changed = true;
    }

    state.members.erase(state.members.begin() + index);
    state.selected_member_index = -1;
    effects.accepted = true;
    effects.changed = true;
    effects.status_should_send = true;
    effects.member_kicked = true;
    effects.member_kicked_id = target_node_id;
    return effects;
}

TeamPageCommandEffects TeamPageCommandReducer::reduceTransferLeader(
    TeamPageCommandState& state,
    uint32_t target_node_id) const
{
    TeamPageCommandEffects effects;
    const int index = findMemberIndex(state, target_node_id);
    if (index < 0)
    {
        return effects;
    }

    for (auto& member : state.members)
    {
        member.leader = false;
    }
    state.members[static_cast<size_t>(index)].leader = true;
    state.self_is_leader = false;

    effects.accepted = true;
    effects.changed = true;
    effects.leader_transferred_key_event = true;
    effects.leader_target = target_node_id;
    return effects;
}

std::string TeamPageCommandReducer::formatTeamNameFromId(const TeamId& id)
{
    char buf[16];
    std::snprintf(buf, sizeof(buf), "TEAM-%02X%02X", id[0], id[1]);
    return std::string(buf);
}

void TeamPageCommandReducer::resetTeamMembership(
    TeamPageCommandState& state,
    bool kicked_out) const
{
    state.in_team = false;
    state.pending_join = false;
    state.pending_join_started_s = 0;
    state.kicked_out = kicked_out;
    state.self_is_leader = false;
    state.last_event_seq = 0;
    state.members.clear();
    state.selected_member_index = -1;
    state.has_team_id = false;
    state.team_id = TeamId{};
    state.team_name.clear();
    state.security_round = 0;
    state.last_update_s = 0;
    state.team_psk = {};
    state.has_team_psk = false;
    state.waiting_new_keys = false;
    resetPairing(state);
}

void TeamPageCommandReducer::resetPairing(
    TeamPageCommandState& state) const
{
    state.pairing_role = TeamPairingRole::None;
    state.pairing_state = TeamPairingState::Idle;
    state.pairing_peer_id = 0;
    state.pairing_team_name.clear();
}

void TeamPageCommandReducer::ensureSelfMember(TeamPageCommandState& state,
                                              bool leader) const
{
    const int index = findMemberIndex(state, 0);
    if (index >= 0)
    {
        auto& self = state.members[static_cast<size_t>(index)];
        self.name = "You";
        self.leader = leader;
        if (self.last_seen_s == 0)
        {
            self.last_seen_s = context_.now_s;
        }
        assignMemberColor(self);
        return;
    }

    TeamMemberUi self;
    self.node_id = 0;
    self.name = "You";
    self.leader = leader;
    self.last_seen_s = context_.now_s;
    assignMemberColor(self);
    state.members.push_back(self);
}

int TeamPageCommandReducer::findMemberIndex(
    const TeamPageCommandState& state,
    uint32_t node_id) const
{
    for (size_t i = 0; i < state.members.size(); ++i)
    {
        if (state.members[i].node_id == node_id)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void TeamPageCommandReducer::assignMemberColor(TeamMemberUi& member) const
{
    uint32_t node_id = member.node_id;
    if (node_id == 0)
    {
        node_id = context_.self_node_id;
    }
    member.color_index = team_color_index_from_node_id(node_id);
}

} // namespace ui
} // namespace team
