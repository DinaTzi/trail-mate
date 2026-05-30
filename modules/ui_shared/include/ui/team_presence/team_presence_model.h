#pragma once

#include "platform/ui/team_ui_types.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ui::team_presence
{

constexpr uint32_t kDefaultOnlineWindowSeconds = 120;
constexpr std::size_t kInvalidTeamMemberIndex = static_cast<std::size_t>(-1);

struct TeamMemberTouchResult
{
    std::size_t index = kInvalidTeamMemberIndex;
    bool created = false;
    bool valid = false;
};

bool isTeamMemberOnline(uint32_t now_s,
                        uint32_t last_seen_s,
                        uint32_t online_window_s = kDefaultOnlineWindowSeconds);

int refreshTeamMemberPresence(std::vector<::team::ui::TeamMemberUi>& members,
                              uint32_t now_s,
                              uint32_t online_window_s = kDefaultOnlineWindowSeconds);

std::size_t findTeamMemberIndex(
    const std::vector<::team::ui::TeamMemberUi>& members,
    uint32_t node_id);

TeamMemberTouchResult touchTeamMember(
    std::vector<::team::ui::TeamMemberUi>& members,
    uint32_t node_id,
    uint32_t seen_s);

uint32_t normalizeSeenSeconds(uint32_t observed_s, uint32_t fallback_now_s);

} // namespace ui::team_presence
