#include "ui/screens/team/team_page_command_reducer.h"

#include <cassert>

namespace
{

team::TeamId testTeamId()
{
    team::TeamId id{};
    id[0] = 0xCA;
    id[1] = 0xFE;
    return id;
}

std::array<uint8_t, team::proto::kTeamChannelPskSize> testPsk(uint8_t seed)
{
    std::array<uint8_t, team::proto::kTeamChannelPskSize> psk{};
    for (size_t i = 0; i < psk.size(); ++i)
    {
        psk[i] = static_cast<uint8_t>(seed + i);
    }
    return psk;
}

team::ui::TeamPageCommandReducer makeReducer()
{
    team::ui::TeamPageCommandContext context;
    context.now_s = 1234;
    context.self_node_id = 0x11111111;
    return team::ui::TeamPageCommandReducer(context);
}

team::ui::TeamMemberUi makeMember(uint32_t node_id,
                                  const char* name,
                                  bool leader = false)
{
    team::ui::TeamMemberUi member;
    member.node_id = node_id;
    member.name = name;
    member.leader = leader;
    member.last_seen_s = 900;
    return member;
}

team::ui::TeamPageCommandState makeInTeamState()
{
    team::ui::TeamPageCommandState state;
    state.in_team = true;
    state.self_is_leader = true;
    state.has_team_id = true;
    state.team_id = testTeamId();
    state.team_name = "Trail";
    state.security_round = 7;
    state.has_team_psk = true;
    state.team_psk = testPsk(0x20);
    state.waiting_new_keys = true;
    state.last_event_seq = 8;
    state.members.push_back(makeMember(0, "You", true));
    state.members.push_back(makeMember(0x22222222, "Ada"));
    state.members.push_back(makeMember(0x33333333, "Ben"));
    return state;
}

void testCreateInitializesLocalLeaderState()
{
    auto reducer = makeReducer();
    team::ui::TeamPageCommandState state;
    team::ui::TeamPageGeneratedTeamSecrets generated;
    generated.team_id = testTeamId();
    generated.team_psk = testPsk(0x10);

    const auto effects = reducer.reduceCreate(state, generated);

    assert(effects.accepted);
    assert(effects.changed);
    assert(effects.team_created_key_event);
    assert(effects.keys_changed);
    assert(state.in_team);
    assert(!state.pending_join);
    assert(!state.kicked_out);
    assert(state.self_is_leader);
    assert(state.has_team_id);
    assert(state.team_name == "TEAM-CAFE");
    assert(state.security_round == 1);
    assert(state.has_team_psk);
    assert(state.team_psk[0] == 0x10);
    assert(state.last_update_s == 1234);
    assert(state.members.size() == 1);
    assert(state.members[0].node_id == 0);
    assert(state.members[0].name == "You");
    assert(state.members[0].leader);
    assert(state.members[0].color_index ==
           team::ui::team_color_index_from_node_id(0x11111111));
}

void testLeaveClearsLocalMembershipAndRequestsSideEffects()
{
    auto reducer = makeReducer();
    auto state = makeInTeamState();
    state.pending_join = true;
    state.pairing_role = team::TeamPairingRole::Leader;
    state.pairing_state = team::TeamPairingState::LeaderBeacon;

    const auto effects = reducer.reduceLeave(state);

    assert(effects.accepted);
    assert(effects.clear_keys);
    assert(effects.stop_pairing);
    assert(effects.clear_keydist_pending);
    assert(effects.clear_status_pending);
    assert(!state.in_team);
    assert(!state.pending_join);
    assert(!state.kicked_out);
    assert(!state.has_team_id);
    assert(!state.has_team_psk);
    assert(state.security_round == 0);
    assert(state.last_event_seq == 0);
    assert(state.members.empty());
    assert(state.pairing_role == team::TeamPairingRole::None);
    assert(state.pairing_state == team::TeamPairingState::Idle);
}

void testKickedOutKeepsKickedFlag()
{
    auto reducer = makeReducer();
    auto state = makeInTeamState();

    const auto effects = reducer.reduceKickedOut(state);

    assert(effects.accepted);
    assert(effects.clear_keys);
    assert(effects.stop_pairing);
    assert(state.kicked_out);
    assert(!state.in_team);
    assert(state.members.empty());
}

void testPairingStartedRecordsPendingJoin()
{
    auto reducer = makeReducer();
    team::ui::TeamPageCommandState state;

    const auto effects = reducer.reducePairingStarted(
        state,
        team::TeamPairingRole::Member,
        team::TeamPairingState::MemberScanning);

    assert(effects.accepted);
    assert(state.pending_join);
    assert(state.pending_join_started_s == 1234);
    assert(state.pairing_role == team::TeamPairingRole::Member);
    assert(state.pairing_state == team::TeamPairingState::MemberScanning);
}

void testJoinCancelStopsTransientPairingOnly()
{
    auto reducer = makeReducer();
    auto state = makeInTeamState();
    state.pending_join = true;
    state.pending_join_started_s = 22;
    state.pairing_role = team::TeamPairingRole::Member;
    state.pairing_state = team::TeamPairingState::WaitingKey;

    const auto effects = reducer.reduceJoinCanceled(state);

    assert(effects.accepted);
    assert(effects.reset_controller_ui);
    assert(effects.stop_pairing);
    assert(state.in_team);
    assert(!state.pending_join);
    assert(state.pending_join_started_s == 0);
    assert(state.pairing_role == team::TeamPairingRole::None);
    assert(state.pairing_state == team::TeamPairingState::Idle);
    assert(state.has_team_id);
}

void testKickConfirmedRotatesKeysAndRemovesMember()
{
    auto reducer = makeReducer();
    auto state = makeInTeamState();
    state.selected_member_index = 1;
    team::ui::TeamPageKickRotation rotation;
    rotation.rotate_keys = true;
    rotation.key_id = 9;
    rotation.team_psk = testPsk(0x40);

    const auto effects =
        reducer.reduceKickConfirmed(state, 0x22222222, rotation);

    assert(effects.accepted);
    assert(effects.keys_changed);
    assert(effects.status_should_send);
    assert(effects.member_kicked);
    assert(effects.member_kicked_id == 0x22222222);
    assert(state.security_round == 9);
    assert(state.team_psk[0] == 0x40);
    assert(!state.waiting_new_keys);
    assert(state.selected_member_index == -1);
    assert(state.members.size() == 2);
    assert(state.members[1].node_id == 0x33333333);
}

void testKickMissingMemberIsIgnored()
{
    auto reducer = makeReducer();
    auto state = makeInTeamState();

    const auto effects =
        reducer.reduceKickConfirmed(state, 0x99999999, {});

    assert(!effects.accepted);
    assert(state.members.size() == 3);
    assert(state.security_round == 7);
}

void testTransferLeaderUpdatesLocalLeader()
{
    auto reducer = makeReducer();
    auto state = makeInTeamState();

    const auto effects =
        reducer.reduceTransferLeader(state, 0x33333333);

    assert(effects.accepted);
    assert(effects.leader_transferred_key_event);
    assert(effects.leader_target == 0x33333333);
    assert(!state.self_is_leader);
    assert(!state.members[0].leader);
    assert(!state.members[1].leader);
    assert(state.members[2].leader);
}

} // namespace

int main()
{
    testCreateInitializesLocalLeaderState();
    testLeaveClearsLocalMembershipAndRequestsSideEffects();
    testKickedOutKeepsKickedFlag();
    testPairingStartedRecordsPendingJoin();
    testJoinCancelStopsTransientPairingOnly();
    testKickConfirmedRotatesKeysAndRemovesMember();
    testKickMissingMemberIsIgnored();
    testTransferLeaderUpdatesLocalLeader();
    return 0;
}
