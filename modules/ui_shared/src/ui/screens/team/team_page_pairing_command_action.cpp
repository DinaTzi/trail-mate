#include "ui/screens/team/team_page_pairing_command_action.h"

namespace team
{
namespace ui
{
namespace
{

TeamPagePairingCommandFailure makeFailure(
    TeamPagePairingCommandRole role,
    TeamPagePairingCommandFailureKind kind)
{
    TeamPagePairingCommandFailure failure;
    failure.role = role;
    failure.kind = kind;
    return failure;
}

std::string teamNameForLeader(const TeamPageCommandState& state)
{
    if (!state.team_name.empty())
    {
        return state.team_name;
    }
    return TeamPageCommandReducer::formatTeamNameFromId(state.team_id);
}

} // namespace

TeamPagePairingCommandEffects TeamPagePairingCommandAction::startPairing(
    TeamPageCommandState& state,
    const TeamPageCommandReducer& reducer,
    const TeamPageRuntimePort& runtime,
    TeamPagePairingCommandRole role,
    uint32_t self_node_id) const
{
    TeamPagePairingCommandEffects effects;
    if (role == TeamPagePairingCommandRole::Leader)
    {
        if (!state.self_is_leader)
        {
            effects.failures.push_back(makeFailure(
                role,
                TeamPagePairingCommandFailureKind::LeaderRequired));
            return effects;
        }
        if (!runtime.hasPairing() || !state.has_team_id ||
            !state.has_team_psk)
        {
            effects.failures.push_back(makeFailure(
                role,
                TeamPagePairingCommandFailureKind::PairingNotReady));
            return effects;
        }

        const std::string team_name = teamNameForLeader(state);
        if (!runtime.startLeader(state.team_id,
                                 state.security_round,
                                 state.team_psk.data(),
                                 state.team_psk.size(),
                                 self_node_id,
                                 team_name.c_str()))
        {
            effects.failures.push_back(makeFailure(
                role,
                TeamPagePairingCommandFailureKind::PairingInitFailed));
            return effects;
        }

        effects.started_pairing = true;
        effects.command = reducer.reducePairingStarted(
            state,
            TeamPairingRole::Leader,
            TeamPairingState::LeaderBeacon);
        effects.accepted = effects.command.accepted;
        return effects;
    }

    if (!runtime.hasPairing())
    {
        effects.failures.push_back(makeFailure(
            role,
            TeamPagePairingCommandFailureKind::PairingNotAvailable));
        return effects;
    }
    if (!runtime.startMember(self_node_id))
    {
        effects.failures.push_back(makeFailure(
            role,
            TeamPagePairingCommandFailureKind::PairingInitFailed));
        return effects;
    }

    effects.started_pairing = true;
    effects.command = reducer.reducePairingStarted(
        state,
        TeamPairingRole::Member,
        TeamPairingState::MemberScanning);
    effects.accepted = effects.command.accepted;
    return effects;
}

} // namespace ui
} // namespace team
