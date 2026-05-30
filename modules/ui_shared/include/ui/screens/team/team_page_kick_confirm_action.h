#pragma once

#include "team/usecase/team_service.h"
#include "ui/screens/team/team_page_command_reducer.h"
#include "ui/screens/team/team_page_runtime_port.h"

#include <cstdint>
#include <vector>

namespace team
{
namespace ui
{

class ITeamPageKickConfirmRandom
{
  public:
    virtual ~ITeamPageKickConfirmRandom() = default;
    virtual uint8_t nextByte() = 0;
};

class ITeamPageKickConfirmDeferred
{
  public:
    virtual ~ITeamPageKickConfirmDeferred() = default;
    virtual void enqueueKeyDist(uint32_t node_id, uint32_t key_id) = 0;
};

enum class TeamPageKickConfirmFailureAction : uint8_t
{
    Kick,
    KeyDist,
    Keys,
    Status
};

enum class TeamPageKickConfirmFailureKind : uint8_t
{
    SendFailed,
    SendFailedDetail
};

struct TeamPageKickConfirmFailure
{
    TeamPageKickConfirmFailureAction action =
        TeamPageKickConfirmFailureAction::Status;
    TeamPageKickConfirmFailureKind kind =
        TeamPageKickConfirmFailureKind::SendFailed;
    bool needs_keys = false;
    team::TeamService::SendError error = team::TeamService::SendError::None;
};

struct TeamPageKickConfirmEffects
{
    TeamPageCommandEffects command;
    std::vector<TeamPageKickConfirmFailure> failures;
    bool accepted = false;
    bool sent_kick = false;
    bool sent_keydist = false;
    bool enqueued_keydist = false;
    bool applied_keys = false;
    bool sent_status = false;
};

class TeamPageKickConfirmAction
{
  public:
    TeamPageKickConfirmEffects confirmKick(
        TeamPageCommandState& state,
        const TeamPageCommandReducer& reducer,
        const TeamPageRuntimePort& runtime,
        ITeamPageKickConfirmRandom& random,
        ITeamPageKickConfirmDeferred& deferred,
        uint32_t self_node_id) const;
};

} // namespace ui
} // namespace team
