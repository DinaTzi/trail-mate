#include "ui/screens/team/team_page_key_event_log.h"

#include <vector>

namespace team
{
namespace ui
{
namespace
{

void writeU32Le(std::vector<uint8_t>& out, uint32_t value)
{
    out.push_back(static_cast<uint8_t>(value & 0xFFu));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFFu));
    out.push_back(static_cast<uint8_t>((value >> 16) & 0xFFu));
    out.push_back(static_cast<uint8_t>((value >> 24) & 0xFFu));
}

void writeU64Le(std::vector<uint8_t>& out, uint64_t value)
{
    for (int i = 0; i < 8; ++i)
    {
        out.push_back(static_cast<uint8_t>((value >> (8 * i)) & 0xFFu));
    }
}

} // namespace

TeamPageKeyEventLog::TeamPageKeyEventLog(ITeamPageKeyEventWriter& writer,
                                         uint32_t timestamp_s)
    : writer_(writer),
      timestamp_s_(timestamp_s)
{
}

bool TeamPageKeyEventLog::appendTeamCreated(TeamPageKeyEventState& state,
                                            uint32_t leader_id) const
{
    std::vector<uint8_t> payload;
    writeU64Le(payload, teamIdToU64(state.team_id));
    writeU32Le(payload, leader_id);
    writeU32Le(payload, state.security_round);
    return append(state,
                  TeamKeyEventType::TeamCreated,
                  payload.data(),
                  payload.size());
}

bool TeamPageKeyEventLog::appendMemberAccepted(
    TeamPageKeyEventState& state,
    uint32_t member_id,
    uint8_t role) const
{
    std::vector<uint8_t> payload;
    writeU32Le(payload, member_id);
    payload.push_back(role);
    return append(state,
                  TeamKeyEventType::MemberAccepted,
                  payload.data(),
                  payload.size());
}

bool TeamPageKeyEventLog::appendMemberKicked(
    TeamPageKeyEventState& state,
    uint32_t member_id) const
{
    std::vector<uint8_t> payload;
    writeU32Le(payload, member_id);
    return append(state,
                  TeamKeyEventType::MemberKicked,
                  payload.data(),
                  payload.size());
}

bool TeamPageKeyEventLog::appendLeaderTransferred(
    TeamPageKeyEventState& state,
    uint32_t leader_id) const
{
    std::vector<uint8_t> payload;
    writeU32Le(payload, leader_id);
    return append(state,
                  TeamKeyEventType::LeaderTransferred,
                  payload.data(),
                  payload.size());
}

bool TeamPageKeyEventLog::appendEpochRotated(
    TeamPageKeyEventState& state,
    uint32_t key_id) const
{
    std::vector<uint8_t> payload;
    writeU32Le(payload, key_id);
    return append(state,
                  TeamKeyEventType::EpochRotated,
                  payload.data(),
                  payload.size());
}

uint64_t TeamPageKeyEventLog::teamIdToU64(const TeamId& id)
{
    uint64_t value = 0;
    for (size_t i = 0; i < id.size(); ++i)
    {
        value |= (static_cast<uint64_t>(id[i]) << (8 * i));
    }
    return value;
}

bool TeamPageKeyEventLog::append(TeamPageKeyEventState& state,
                                 TeamKeyEventType type,
                                 const uint8_t* payload,
                                 size_t payload_size) const
{
    if (!state.has_team_id)
    {
        return false;
    }

    const uint32_t next_seq = state.last_event_seq + 1;
    if (!writer_.appendKeyEvent(state.team_id,
                                type,
                                next_seq,
                                timestamp_s_,
                                payload,
                                payload_size))
    {
        return false;
    }

    state.last_event_seq = next_seq;
    return true;
}

} // namespace ui
} // namespace team
