#pragma once

#include "ui/screens/team/team_page_command_reducer.h"
#include "ui/screens/team/team_page_runtime_port.h"

namespace team
{
namespace ui
{

struct TeamPageRequestKeysEffects
{
    bool accepted = false;
    bool ignored = false;
    bool sent_request = false;
    bool send_failed = false;
    team::TeamService::SendError error = team::TeamService::SendError::None;
};

class TeamPageRequestKeysAction
{
  public:
    TeamPageRequestKeysEffects requestKeys(
        const TeamPageCommandState& state,
        const TeamPageRuntimePort& runtime,
        uint32_t self_node_id) const;
};

} // namespace ui
} // namespace team
