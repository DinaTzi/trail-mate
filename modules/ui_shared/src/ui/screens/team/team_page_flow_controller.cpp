#include "ui/screens/team/team_page_flow_controller.h"

namespace team
{
namespace ui
{

TeamPageFlowResult TeamPageFlowController::navigateTo(
    TeamPageFlowState& state,
    TeamPage page,
    bool push) const
{
    TeamPageFlowResult result;
    if (push && state.page != page)
    {
        state.nav_stack.push_back(state.page);
    }
    result.changed = state.page != page;
    state.page = page;
    return result;
}

TeamPageFlowResult TeamPageFlowController::navigateBack(
    TeamPageFlowState& state) const
{
    if (state.page == TeamPage::JoinPending && state.in_team)
    {
        return resetTo(state, TeamPage::StatusInTeam);
    }

    if (!state.nav_stack.empty())
    {
        TeamPage next_page = state.nav_stack.back();
        state.nav_stack.pop_back();
        if (next_page == TeamPage::StatusNotInTeam && state.in_team)
        {
            next_page = TeamPage::StatusInTeam;
        }
        const bool changed = state.page != next_page;
        state.page = next_page;
        TeamPageFlowResult result;
        result.changed = changed || true;
        return result;
    }

    TeamPageFlowResult result;
    result.request_exit = true;
    return result;
}

TeamPageFlowResult TeamPageFlowController::resetTo(
    TeamPageFlowState& state,
    TeamPage page) const
{
    TeamPageFlowResult result;
    result.changed = state.page != page || !state.nav_stack.empty();
    state.nav_stack.clear();
    state.page = page;
    return result;
}

TeamPageFlowResult TeamPageFlowController::selectInitialPage(
    TeamPageFlowState& state) const
{
    TeamPage next_page = TeamPage::StatusNotInTeam;
    if (state.kicked_out)
    {
        next_page = TeamPage::KickedOut;
    }
    else if (isPairingActive(state.pairing_state) || state.pending_join)
    {
        next_page = TeamPage::JoinPending;
    }
    else if (state.in_team)
    {
        next_page = TeamPage::StatusInTeam;
    }

    TeamPageFlowResult result;
    result.changed = state.page != next_page;
    state.page = next_page;
    return result;
}

TeamPageFlowResult TeamPageFlowController::syncRuntime(
    TeamPageFlowState& state,
    uint32_t now_s) const
{
    TeamPageFlowResult result;
    const bool pairing_active = isPairingActive(state.pairing_state);
    if (pairing_active && state.pending_join_started_s == 0)
    {
        state.pending_join_started_s = now_s;
        result.changed = true;
    }
    if (pairing_active && !state.in_team &&
        state.page != TeamPage::JoinPending)
    {
        state.page = TeamPage::JoinPending;
        state.nav_stack.clear();
        result.changed = true;
    }
    if (!pairing_active && state.in_team &&
        (state.page == TeamPage::StatusNotInTeam ||
         state.page == TeamPage::JoinPending))
    {
        state.page = TeamPage::StatusInTeam;
        state.nav_stack.clear();
        result.changed = true;
    }
    if (!pairing_active && !state.in_team &&
        state.page == TeamPage::JoinPending)
    {
        state.page = TeamPage::StatusNotInTeam;
        state.nav_stack.clear();
        result.changed = true;
    }
    return result;
}

bool TeamPageFlowController::isPairingActive(TeamPairingState state)
{
    return state != TeamPairingState::Idle &&
           state != TeamPairingState::Completed &&
           state != TeamPairingState::Failed;
}

} // namespace ui
} // namespace team
