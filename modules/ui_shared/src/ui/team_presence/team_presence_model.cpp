#include "ui/team_presence/team_presence_model.h"

namespace ui::team_presence
{

bool isTeamMemberOnline(uint32_t now_s,
                        uint32_t last_seen_s,
                        uint32_t online_window_s)
{
    if (last_seen_s == 0)
    {
        return false;
    }
    if (last_seen_s >= now_s)
    {
        return true;
    }
    return (now_s - last_seen_s) <= online_window_s;
}

int refreshTeamMemberPresence(std::vector<::team::ui::TeamMemberUi>& members,
                              uint32_t now_s,
                              uint32_t online_window_s)
{
    int online_count = 0;
    for (auto& member : members)
    {
        member.online = isTeamMemberOnline(now_s,
                                           member.last_seen_s,
                                           online_window_s);
        if (member.online)
        {
            ++online_count;
        }
    }
    return online_count;
}

std::size_t findTeamMemberIndex(
    const std::vector<::team::ui::TeamMemberUi>& members,
    uint32_t node_id)
{
    for (std::size_t i = 0; i < members.size(); ++i)
    {
        if (members[i].node_id == node_id)
        {
            return i;
        }
    }
    return kInvalidTeamMemberIndex;
}

TeamMemberTouchResult touchTeamMember(
    std::vector<::team::ui::TeamMemberUi>& members,
    uint32_t node_id,
    uint32_t seen_s)
{
    std::size_t index = findTeamMemberIndex(members, node_id);
    if (index == kInvalidTeamMemberIndex)
    {
        ::team::ui::TeamMemberUi member;
        member.node_id = node_id;
        member.last_seen_s = seen_s;
        members.push_back(member);
        TeamMemberTouchResult result;
        result.index = members.size() - 1U;
        result.created = true;
        result.valid = true;
        return result;
    }

    members[index].last_seen_s = seen_s;
    TeamMemberTouchResult result;
    result.index = index;
    result.valid = true;
    return result;
}

uint32_t normalizeSeenSeconds(uint32_t observed_s, uint32_t fallback_now_s)
{
    return observed_s != 0 ? observed_s : fallback_now_s;
}

} // namespace ui::team_presence
