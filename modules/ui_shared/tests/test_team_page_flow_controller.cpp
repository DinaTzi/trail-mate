#include "ui/screens/team/team_page_flow_controller.h"

#include <cassert>

namespace
{

void testNavigateToPushesPreviousPage()
{
    team::ui::TeamPageFlowState state;
    state.page = team::ui::TeamPage::StatusInTeam;

    const auto result = team::ui::TeamPageFlowController().navigateTo(
        state,
        team::ui::TeamPage::TeamHome,
        true);

    assert(result.changed);
    assert(state.page == team::ui::TeamPage::TeamHome);
    assert(state.nav_stack.size() == 1);
    assert(state.nav_stack[0] == team::ui::TeamPage::StatusInTeam);
}

void testNavigateBackNormalizesNotInTeamWhenAlreadyInTeam()
{
    team::ui::TeamPageFlowState state;
    state.page = team::ui::TeamPage::TeamHome;
    state.in_team = true;
    state.nav_stack.push_back(team::ui::TeamPage::StatusNotInTeam);

    const auto result =
        team::ui::TeamPageFlowController().navigateBack(state);

    assert(result.changed);
    assert(!result.request_exit);
    assert(state.page == team::ui::TeamPage::StatusInTeam);
    assert(state.nav_stack.empty());
}

void testNavigateBackRequestsExitWhenStackEmpty()
{
    team::ui::TeamPageFlowState state;
    state.page = team::ui::TeamPage::StatusNotInTeam;

    const auto result =
        team::ui::TeamPageFlowController().navigateBack(state);

    assert(!result.changed);
    assert(result.request_exit);
}

void testJoinPendingBackResetsToInTeamStatus()
{
    team::ui::TeamPageFlowState state;
    state.page = team::ui::TeamPage::JoinPending;
    state.in_team = true;
    state.nav_stack.push_back(team::ui::TeamPage::TeamHome);

    const auto result =
        team::ui::TeamPageFlowController().navigateBack(state);

    assert(result.changed);
    assert(state.page == team::ui::TeamPage::StatusInTeam);
    assert(state.nav_stack.empty());
}

void testInitialPageSelection()
{
    team::ui::TeamPageFlowState state;
    state.kicked_out = true;
    team::ui::TeamPageFlowController controller;
    controller.selectInitialPage(state);
    assert(state.page == team::ui::TeamPage::KickedOut);

    state = {};
    state.pending_join = true;
    controller.selectInitialPage(state);
    assert(state.page == team::ui::TeamPage::JoinPending);

    state = {};
    state.in_team = true;
    controller.selectInitialPage(state);
    assert(state.page == team::ui::TeamPage::StatusInTeam);

    state = {};
    controller.selectInitialPage(state);
    assert(state.page == team::ui::TeamPage::StatusNotInTeam);
}

void testSyncRuntimeKeepsPageConsistentWithPairingAndMembership()
{
    team::ui::TeamPageFlowController controller;
    team::ui::TeamPageFlowState state;
    state.pairing_state = team::TeamPairingState::MemberScanning;
    state.page = team::ui::TeamPage::StatusNotInTeam;

    auto result = controller.syncRuntime(state, 50);

    assert(result.changed);
    assert(state.pending_join_started_s == 50);
    assert(state.page == team::ui::TeamPage::JoinPending);

    state.in_team = true;
    state.pairing_state = team::TeamPairingState::Idle;
    result = controller.syncRuntime(state, 60);
    assert(result.changed);
    assert(state.page == team::ui::TeamPage::StatusInTeam);

    state.in_team = false;
    state.page = team::ui::TeamPage::JoinPending;
    result = controller.syncRuntime(state, 70);
    assert(result.changed);
    assert(state.page == team::ui::TeamPage::StatusNotInTeam);
}

} // namespace

int main()
{
    testNavigateToPushesPreviousPage();
    testNavigateBackNormalizesNotInTeamWhenAlreadyInTeam();
    testNavigateBackRequestsExitWhenStackEmpty();
    testJoinPendingBackResetsToInTeamStatus();
    testInitialPageSelection();
    testSyncRuntimeKeepsPageConsistentWithPairingAndMembership();
    return 0;
}
