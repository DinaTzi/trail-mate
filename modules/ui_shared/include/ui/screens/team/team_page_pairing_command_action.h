#pragma once

#include "ui/screens/team/team_page_command_reducer.h"
#include "ui/screens/team/team_page_runtime_port.h"

#include <cstdint>
#include <vector>

namespace team
{
namespace ui
{

enum class TeamPagePairingCommandRole : uint8_t
{
    Leader,
    Member
};

enum class TeamPagePairingCommandFailureKind : uint8_t
{
    LeaderRequired,
    PairingNotReady,
    PairingNotAvailable,
    PairingInitFailed
};

struct TeamPagePairingCommandFailure
{
    TeamPagePairingCommandRole role = TeamPagePairingCommandRole::Member;
    TeamPagePairingCommandFailureKind kind =
        TeamPagePairingCommandFailureKind::PairingInitFailed;
};

struct TeamPagePairingCommandEffects
{
    TeamPageCommandEffects command;
    std::vector<TeamPagePairingCommandFailure> failures;
    bool accepted = false;
    bool started_pairing = false;
};

class TeamPagePairingCommandAction
{
  public:
    TeamPagePairingCommandEffects startPairing(
        TeamPageCommandState& state,
        const TeamPageCommandReducer& reducer,
        const TeamPageRuntimePort& runtime,
        TeamPagePairingCommandRole role,
        uint32_t self_node_id) const;
};

} // namespace ui
} // namespace team
