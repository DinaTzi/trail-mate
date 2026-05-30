#include "ui/screens/team/team_page_create_team_action.h"

namespace team
{
namespace ui
{
namespace
{

TeamPageCreateTeamFailure makeFailure(
    TeamPageCreateTeamFailureAction action,
    TeamPageCreateTeamFailureKind kind,
    bool needs_keys)
{
    TeamPageCreateTeamFailure failure;
    failure.action = action;
    failure.kind = kind;
    failure.needs_keys = needs_keys;
    return failure;
}

TeamId generateTeamId(ITeamPageCreateTeamRandom& random)
{
    TeamId id{};
    for (size_t i = 0; i < id.size(); ++i)
    {
        id[i] = random.nextByte();
    }
    return id;
}

std::array<uint8_t, team::proto::kTeamChannelPskSize> generateTeamPsk(
    ITeamPageCreateTeamRandom& random)
{
    std::array<uint8_t, team::proto::kTeamChannelPskSize> psk{};
    for (size_t i = 0; i < psk.size(); ++i)
    {
        psk[i] = random.nextByte();
    }
    return psk;
}

} // namespace

TeamPageCreateTeamEffects TeamPageCreateTeamAction::createTeam(
    TeamPageCommandState& state,
    TeamPageKeyEventState& key_event_state,
    const TeamPageCommandReducer& reducer,
    const TeamPageRuntimePort& runtime,
    const TeamPageKeyEventLog& key_log,
    ITeamPageCreateTeamRandom& random,
    uint32_t self_node_id) const
{
    TeamPageCreateTeamEffects effects;

    TeamPageGeneratedTeamSecrets generated;
    if (!state.has_team_id)
    {
        generated.team_id = generateTeamId(random);
    }
    if (!state.has_team_psk)
    {
        generated.team_psk = generateTeamPsk(random);
    }

    effects.command = reducer.reduceCreate(state, generated);
    effects.accepted = effects.command.accepted;
    if (!effects.command.accepted)
    {
        return effects;
    }

    if (effects.command.team_created_key_event)
    {
        key_event_state.team_id = state.team_id;
        key_event_state.has_team_id = state.has_team_id;
        key_event_state.security_round = state.security_round;
        effects.appended_key_event =
            key_log.appendTeamCreated(key_event_state, 0);
    }

    if (effects.command.keys_changed)
    {
        effects.applied_keys = true;
        if (!runtime.setKeysFromPsk(state.team_id,
                                    state.security_round,
                                    state.team_psk.data(),
                                    state.team_psk.size()))
        {
            effects.failures.push_back(makeFailure(
                TeamPageCreateTeamFailureAction::Keys,
                TeamPageCreateTeamFailureKind::SendFailed,
                true));
        }

        effects.saved_keys = runtime.saveKeysNow(state.team_id,
                                                 state.security_round,
                                                 state.team_psk);
    }

    if (!runtime.hasPairing() || !state.has_team_id || !state.has_team_psk)
    {
        effects.failures.push_back(makeFailure(
            TeamPageCreateTeamFailureAction::Pairing,
            TeamPageCreateTeamFailureKind::PairingNotReady,
            false));
        return effects;
    }

    const std::string team_name = state.team_name.empty()
                                      ? TeamPageCommandReducer::formatTeamNameFromId(state.team_id)
                                      : state.team_name;
    if (!runtime.startLeader(state.team_id,
                             state.security_round,
                             state.team_psk.data(),
                             state.team_psk.size(),
                             self_node_id,
                             team_name.c_str()))
    {
        effects.failures.push_back(makeFailure(
            TeamPageCreateTeamFailureAction::Pairing,
            TeamPageCreateTeamFailureKind::PairingInitFailed,
            false));
        return effects;
    }

    effects.started_pairing = true;
    effects.command = reducer.reducePairingStarted(
        state,
        TeamPairingRole::Leader,
        TeamPairingState::LeaderBeacon);
    return effects;
}

} // namespace ui
} // namespace team
