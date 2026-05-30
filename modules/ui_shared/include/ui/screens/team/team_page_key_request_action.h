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

enum class TeamPageKeyRequestFailureKind : uint8_t
{
    Ignored,
    SendFailedDetail
};

struct TeamPageKeyRequestFailure
{
    TeamPageKeyRequestFailureKind kind =
        TeamPageKeyRequestFailureKind::Ignored;
    team::TeamService::SendError error = team::TeamService::SendError::None;
};

struct TeamPageKeyRequestEffects
{
    std::vector<TeamPageKeyRequestFailure> failures;
    bool accepted = false;
    bool sent_keydist = false;
};

class TeamPageKeyRequestAction
{
  public:
    TeamPageKeyRequestEffects handleRequest(
        const TeamPageCommandState& state,
        const team::TeamKeyRequestEvent& event,
        const TeamPageRuntimePort& runtime) const;
};

} // namespace ui
} // namespace team
