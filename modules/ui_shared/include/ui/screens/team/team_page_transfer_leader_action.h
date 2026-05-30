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

enum class TeamPageTransferLeaderFailureAction : uint8_t
{
    Transfer
};

enum class TeamPageTransferLeaderFailureKind : uint8_t
{
    SendFailed
};

struct TeamPageTransferLeaderFailure
{
    TeamPageTransferLeaderFailureAction action =
        TeamPageTransferLeaderFailureAction::Transfer;
    TeamPageTransferLeaderFailureKind kind =
        TeamPageTransferLeaderFailureKind::SendFailed;
    bool needs_keys = false;
    team::TeamService::SendError error = team::TeamService::SendError::None;
};

struct TeamPageTransferLeaderEffects
{
    TeamPageCommandEffects command;
    std::vector<TeamPageTransferLeaderFailure> failures;
    bool accepted = false;
    bool sent_transfer = false;
    bool appended_key_event = false;
};

class TeamPageTransferLeaderAction
{
  public:
    TeamPageTransferLeaderEffects transferLeader(
        TeamPageCommandState& state,
        TeamPageKeyEventState& key_event_state,
        const TeamPageCommandReducer& reducer,
        const TeamPageRuntimePort& runtime,
        const TeamPageKeyEventLog& key_log) const;
};

} // namespace ui
} // namespace team
