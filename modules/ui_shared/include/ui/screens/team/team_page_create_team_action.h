#pragma once

#include "team/usecase/team_service.h"
#include "ui/screens/team/team_page_command_reducer.h"
#include "ui/screens/team/team_page_key_event_log.h"
#include "ui/screens/team/team_page_runtime_port.h"

#include <cstdint>
#include <vector>

namespace team
{
namespace ui
{

class ITeamPageCreateTeamRandom
{
  public:
    virtual ~ITeamPageCreateTeamRandom() = default;
    virtual uint8_t nextByte() = 0;
};

enum class TeamPageCreateTeamFailureAction : uint8_t
{
    Keys,
    Pairing
};

enum class TeamPageCreateTeamFailureKind : uint8_t
{
    SendFailed,
    PairingNotReady,
    PairingInitFailed
};

struct TeamPageCreateTeamFailure
{
    TeamPageCreateTeamFailureAction action =
        TeamPageCreateTeamFailureAction::Pairing;
    TeamPageCreateTeamFailureKind kind =
        TeamPageCreateTeamFailureKind::SendFailed;
    bool needs_keys = false;
    team::TeamService::SendError error = team::TeamService::SendError::None;
};

struct TeamPageCreateTeamEffects
{
    TeamPageCommandEffects command;
    std::vector<TeamPageCreateTeamFailure> failures;
    bool accepted = false;
    bool appended_key_event = false;
    bool applied_keys = false;
    bool saved_keys = false;
    bool started_pairing = false;
};

class TeamPageCreateTeamAction
{
  public:
    TeamPageCreateTeamEffects createTeam(
        TeamPageCommandState& state,
        TeamPageKeyEventState& key_event_state,
        const TeamPageCommandReducer& reducer,
        const TeamPageRuntimePort& runtime,
        const TeamPageKeyEventLog& key_log,
        ITeamPageCreateTeamRandom& random,
        uint32_t self_node_id) const;
};

} // namespace ui
} // namespace team
