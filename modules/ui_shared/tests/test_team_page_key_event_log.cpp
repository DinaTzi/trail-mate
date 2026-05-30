#include "ui/screens/team/team_page_key_event_log.h"

#include <cassert>
#include <vector>

namespace
{

struct CapturedEvent
{
    team::TeamId team_id{};
    team::ui::TeamKeyEventType type = team::ui::TeamKeyEventType::TeamCreated;
    uint32_t seq = 0;
    uint32_t timestamp_s = 0;
    std::vector<uint8_t> payload;
};

class FakeWriter final : public team::ui::ITeamPageKeyEventWriter
{
  public:
    bool appendKeyEvent(const team::TeamId& team_id,
                        team::ui::TeamKeyEventType type,
                        uint32_t event_seq,
                        uint32_t timestamp_s,
                        const uint8_t* payload,
                        size_t payload_size) override
    {
        if (!accept)
        {
            return false;
        }
        CapturedEvent event;
        event.team_id = team_id;
        event.type = type;
        event.seq = event_seq;
        event.timestamp_s = timestamp_s;
        event.payload.assign(payload, payload + payload_size);
        events.push_back(event);
        return true;
    }

    bool accept = true;
    std::vector<CapturedEvent> events;
};

team::TeamId testTeamId()
{
    team::TeamId id{};
    id[0] = 0x12;
    id[1] = 0x34;
    id[2] = 0x56;
    id[3] = 0x78;
    return id;
}

uint32_t readU32(const std::vector<uint8_t>& data, size_t offset)
{
    return static_cast<uint32_t>(data[offset]) |
           (static_cast<uint32_t>(data[offset + 1]) << 8) |
           (static_cast<uint32_t>(data[offset + 2]) << 16) |
           (static_cast<uint32_t>(data[offset + 3]) << 24);
}

uint64_t readU64(const std::vector<uint8_t>& data, size_t offset)
{
    uint64_t value = 0;
    for (size_t i = 0; i < 8; ++i)
    {
        value |= (static_cast<uint64_t>(data[offset + i]) << (8 * i));
    }
    return value;
}

team::ui::TeamPageKeyEventState makeState()
{
    team::ui::TeamPageKeyEventState state;
    state.team_id = testTeamId();
    state.has_team_id = true;
    state.last_event_seq = 4;
    state.security_round = 9;
    return state;
}

void testTeamCreatedPayloadAndSeq()
{
    FakeWriter writer;
    team::ui::TeamPageKeyEventLog log(writer, 77);
    auto state = makeState();

    assert(log.appendTeamCreated(state, 0));
    assert(state.last_event_seq == 5);
    assert(writer.events.size() == 1);
    assert(writer.events[0].type == team::ui::TeamKeyEventType::TeamCreated);
    assert(writer.events[0].seq == 5);
    assert(writer.events[0].timestamp_s == 77);
    assert(readU64(writer.events[0].payload, 0) ==
           team::ui::TeamPageKeyEventLog::teamIdToU64(testTeamId()));
    assert(readU32(writer.events[0].payload, 8) == 0);
    assert(readU32(writer.events[0].payload, 12) == 9);
}

void testMemberEventsPayloads()
{
    FakeWriter writer;
    team::ui::TeamPageKeyEventLog log(writer, 88);
    auto state = makeState();

    assert(log.appendMemberAccepted(state, 0x22222222, 1));
    assert(log.appendMemberKicked(state, 0x33333333));
    assert(log.appendLeaderTransferred(state, 0x44444444));
    assert(log.appendEpochRotated(state, 12));

    assert(state.last_event_seq == 8);
    assert(writer.events.size() == 4);
    assert(writer.events[0].type == team::ui::TeamKeyEventType::MemberAccepted);
    assert(readU32(writer.events[0].payload, 0) == 0x22222222);
    assert(writer.events[0].payload[4] == 1);
    assert(writer.events[1].type == team::ui::TeamKeyEventType::MemberKicked);
    assert(readU32(writer.events[1].payload, 0) == 0x33333333);
    assert(writer.events[2].type == team::ui::TeamKeyEventType::LeaderTransferred);
    assert(readU32(writer.events[2].payload, 0) == 0x44444444);
    assert(writer.events[3].type == team::ui::TeamKeyEventType::EpochRotated);
    assert(readU32(writer.events[3].payload, 0) == 12);
}

void testAppendFailureDoesNotAdvanceSeq()
{
    FakeWriter writer;
    writer.accept = false;
    team::ui::TeamPageKeyEventLog log(writer, 99);
    auto state = makeState();

    assert(!log.appendMemberKicked(state, 0x33333333));
    assert(state.last_event_seq == 4);
    assert(writer.events.empty());

    state.has_team_id = false;
    writer.accept = true;
    assert(!log.appendEpochRotated(state, 12));
    assert(state.last_event_seq == 4);
}

} // namespace

int main()
{
    testTeamCreatedPayloadAndSeq();
    testMemberEventsPayloads();
    testAppendFailureDoesNotAdvanceSeq();
    return 0;
}
