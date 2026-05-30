#pragma once

#include "team/domain/team_types.h"
#include "ui/screens/team/team_page_types.h"

#include <cstdint>
#include <vector>

namespace team
{
namespace ui
{

struct TeamPageFlowState
{
    TeamPage page = TeamPage::StatusNotInTeam;
    std::vector<TeamPage> nav_stack;
    bool in_team = false;
    bool pending_join = false;
    bool kicked_out = false;
    uint32_t pending_join_started_s = 0;
    TeamPairingState pairing_state = TeamPairingState::Idle;
};

struct TeamPageFlowResult
{
    bool changed = false;
    bool request_exit = false;
};

class TeamPageFlowController
{
  public:
    TeamPageFlowResult navigateTo(TeamPageFlowState& state,
                                  TeamPage page,
                                  bool push) const;
    TeamPageFlowResult navigateBack(TeamPageFlowState& state) const;
    TeamPageFlowResult resetTo(TeamPageFlowState& state,
                               TeamPage page) const;
    TeamPageFlowResult selectInitialPage(TeamPageFlowState& state) const;
    TeamPageFlowResult syncRuntime(TeamPageFlowState& state,
                                   uint32_t now_s) const;

    static bool isPairingActive(TeamPairingState state);
};

} // namespace ui
} // namespace team
