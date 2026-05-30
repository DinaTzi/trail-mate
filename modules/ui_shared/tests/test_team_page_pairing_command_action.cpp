#include "ui/screens/team/team_page_pairing_command_action.h"

#include <cassert>
#include <string>

namespace
{

team::TeamId testTeamId()
{
    team::TeamId id{};
    id[0] = 0xCA;
    id[1] = 0xFE;
    return id;
}

std::array<uint8_t, team::proto::kTeamChannelPskSize> testPsk()
{
    std::array<uint8_t, team::proto::kTeamChannelPskSize> psk{};
    for (size_t i = 0; i < psk.size(); ++i)
    {
        psk[i] = static_cast<uint8_t>(0x40 + i);
    }
    return psk;
}

team::ui::TeamPageCommandState makeLeaderState()
{
    team::ui::TeamPageCommandState state;
    state.in_team = true;
    state.self_is_leader = true;
    state.has_team_id = true;
    state.team_id = testTeamId();
    state.team_name = "Trail";
    state.security_round = 9;
    state.has_team_psk = true;
    state.team_psk = testPsk();
    return state;
}

team::ui::TeamPageCommandReducer makeReducer()
{
    team::ui::TeamPageCommandContext context;
    context.now_s = 1234;
    context.self_node_id = 0x11111111;
    return team::ui::TeamPageCommandReducer(context);
}

class FakePairing final : public team::ui::ITeamPagePairingPort
{
  public:
    bool startLeader(const team::TeamId& team_id,
                     uint32_t key_id,
                     const uint8_t* psk,
                     size_t psk_len,
                     uint32_t leader_id,
                     const char* team_name) override
    {
        start_leader_count += 1;
        last_team_id = team_id;
        last_key_id = key_id;
        last_psk0 = psk ? psk[0] : 0;
        last_psk_len = psk_len;
        last_leader_id = leader_id;
        last_team_name = team_name ? team_name : "";
        return start_leader_ok;
    }

    bool startMember(uint32_t self_id) override
    {
        start_member_count += 1;
        last_member_self_id = self_id;
        return start_member_ok;
    }

    void stop() override {}
    team::TeamPairingStatus status() const override { return {}; }

    bool start_leader_ok = true;
    bool start_member_ok = true;
    int start_leader_count = 0;
    int start_member_count = 0;
    team::TeamId last_team_id{};
    uint32_t last_key_id = 0;
    uint8_t last_psk0 = 0;
    size_t last_psk_len = 0;
    uint32_t last_leader_id = 0;
    uint32_t last_member_self_id = 0;
    std::string last_team_name;
};

void testLeaderStartsPairingAndReducesPendingState()
{
    auto state = makeLeaderState();
    FakePairing pairing;
    team::ui::TeamPageRuntimePort runtime(nullptr, &pairing, nullptr);

    const auto effects =
        team::ui::TeamPagePairingCommandAction().startPairing(
            state,
            makeReducer(),
            runtime,
            team::ui::TeamPagePairingCommandRole::Leader,
            0x11111111);

    assert(effects.accepted);
    assert(effects.started_pairing);
    assert(effects.failures.empty());
    assert(pairing.start_leader_count == 1);
    assert(pairing.last_team_id == testTeamId());
    assert(pairing.last_key_id == 9);
    assert(pairing.last_psk0 == 0x40);
    assert(pairing.last_psk_len == team::proto::kTeamChannelPskSize);
    assert(pairing.last_leader_id == 0x11111111);
    assert(pairing.last_team_name == "Trail");
    assert(state.pending_join);
    assert(state.pending_join_started_s == 1234);
    assert(state.pairing_role == team::TeamPairingRole::Leader);
    assert(state.pairing_state == team::TeamPairingState::LeaderBeacon);
}

void testLeaderUsesFallbackName()
{
    auto state = makeLeaderState();
    state.team_name.clear();
    FakePairing pairing;
    team::ui::TeamPageRuntimePort runtime(nullptr, &pairing, nullptr);

    const auto effects =
        team::ui::TeamPagePairingCommandAction().startPairing(
            state,
            makeReducer(),
            runtime,
            team::ui::TeamPagePairingCommandRole::Leader,
            0x11111111);

    assert(effects.accepted);
    assert(pairing.last_team_name == "TEAM-CAFE");
}

void testLeaderValidationFailuresDoNotCallRuntime()
{
    auto state = makeLeaderState();
    state.self_is_leader = false;
    FakePairing pairing;
    team::ui::TeamPageRuntimePort runtime(nullptr, &pairing, nullptr);

    const auto effects =
        team::ui::TeamPagePairingCommandAction().startPairing(
            state,
            makeReducer(),
            runtime,
            team::ui::TeamPagePairingCommandRole::Leader,
            0x11111111);

    assert(!effects.accepted);
    assert(!effects.started_pairing);
    assert(effects.failures.size() == 1);
    assert(effects.failures[0].kind ==
           team::ui::TeamPagePairingCommandFailureKind::LeaderRequired);
    assert(pairing.start_leader_count == 0);

    state = makeLeaderState();
    state.has_team_psk = false;
    const auto missing_keys =
        team::ui::TeamPagePairingCommandAction().startPairing(
            state,
            makeReducer(),
            runtime,
            team::ui::TeamPagePairingCommandRole::Leader,
            0x11111111);

    assert(!missing_keys.accepted);
    assert(missing_keys.failures.size() == 1);
    assert(missing_keys.failures[0].kind ==
           team::ui::TeamPagePairingCommandFailureKind::PairingNotReady);
    assert(pairing.start_leader_count == 0);
}

void testMemberStartsPairingAndReducesPendingState()
{
    team::ui::TeamPageCommandState state;
    FakePairing pairing;
    team::ui::TeamPageRuntimePort runtime(nullptr, &pairing, nullptr);

    const auto effects =
        team::ui::TeamPagePairingCommandAction().startPairing(
            state,
            makeReducer(),
            runtime,
            team::ui::TeamPagePairingCommandRole::Member,
            0x11111111);

    assert(effects.accepted);
    assert(effects.started_pairing);
    assert(effects.failures.empty());
    assert(pairing.start_member_count == 1);
    assert(pairing.last_member_self_id == 0x11111111);
    assert(state.pending_join);
    assert(state.pending_join_started_s == 1234);
    assert(state.pairing_role == team::TeamPairingRole::Member);
    assert(state.pairing_state == team::TeamPairingState::MemberScanning);
}

void testMemberMissingPairingAndInitFailureAreReported()
{
    team::ui::TeamPageCommandState state;
    team::ui::TeamPageRuntimePort missing_runtime(nullptr, nullptr, nullptr);

    const auto missing =
        team::ui::TeamPagePairingCommandAction().startPairing(
            state,
            makeReducer(),
            missing_runtime,
            team::ui::TeamPagePairingCommandRole::Member,
            0x11111111);

    assert(!missing.accepted);
    assert(missing.failures.size() == 1);
    assert(missing.failures[0].kind ==
           team::ui::TeamPagePairingCommandFailureKind::PairingNotAvailable);

    FakePairing pairing;
    pairing.start_member_ok = false;
    team::ui::TeamPageRuntimePort runtime(nullptr, &pairing, nullptr);
    const auto failed =
        team::ui::TeamPagePairingCommandAction().startPairing(
            state,
            makeReducer(),
            runtime,
            team::ui::TeamPagePairingCommandRole::Member,
            0x11111111);

    assert(!failed.accepted);
    assert(failed.failures.size() == 1);
    assert(failed.failures[0].kind ==
           team::ui::TeamPagePairingCommandFailureKind::PairingInitFailed);
    assert(pairing.start_member_count == 1);
}

} // namespace

int main()
{
    testLeaderStartsPairingAndReducesPendingState();
    testLeaderUsesFallbackName();
    testLeaderValidationFailuresDoNotCallRuntime();
    testMemberStartsPairingAndReducesPendingState();
    testMemberMissingPairingAndInitFailureAreReported();
    return 0;
}
