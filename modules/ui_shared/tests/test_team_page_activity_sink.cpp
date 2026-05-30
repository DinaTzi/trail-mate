#include "ui/screens/team/team_page_activity_sink.h"

#include "team/protocol/team_position.h"

#include <cassert>

namespace
{

struct PositionSample
{
    team::TeamId team_id{};
    uint32_t member_id = 0;
    int32_t lat_e7 = 0;
    int32_t lon_e7 = 0;
    int16_t alt_m = 0;
    uint16_t speed_dmps = 0;
    uint32_t ts = 0;
};

struct ChatSample
{
    team::TeamId team_id{};
    uint32_t peer_id = 0;
    bool incoming = false;
    uint32_t ts = 0;
    team::proto::TeamChatType type = team::proto::TeamChatType::Text;
    std::vector<uint8_t> payload;
};

class FakePositionLog final : public team::ui::ITeamPagePositionLog
{
  public:
    bool appendPosition(const team::TeamId& team_id,
                        uint32_t member_id,
                        int32_t lat_e7,
                        int32_t lon_e7,
                        int16_t alt_m,
                        uint16_t speed_dmps,
                        uint32_t ts) override
    {
        positions.push_back({team_id,
                             member_id,
                             lat_e7,
                             lon_e7,
                             alt_m,
                             speed_dmps,
                             ts});
        return true;
    }

    bool appendMemberTrack(
        const team::TeamId& team_id,
        uint32_t member_id,
        const team::proto::TeamTrackMessage& track) override
    {
        track_team_id = team_id;
        track_member_id = member_id;
        last_track = track;
        append_track_count += 1;
        return true;
    }

    bool memberTrackPath(const team::TeamId&,
                         uint32_t,
                         std::string& out_path) override
    {
        path_requested_count += 1;
        if (!path_ok)
        {
            return false;
        }
        out_path = track_path;
        return true;
    }

    std::vector<PositionSample> positions;
    int append_track_count = 0;
    int path_requested_count = 0;
    bool path_ok = true;
    std::string track_path = "/tmp/member.trk";
    team::TeamId track_team_id{};
    uint32_t track_member_id = 0;
    team::proto::TeamTrackMessage last_track;
};

class FakeChatLog final : public team::ui::ITeamPageChatLog
{
  public:
    bool appendStructuredChat(const team::TeamId& team_id,
                              uint32_t peer_id,
                              bool incoming,
                              uint32_t ts,
                              team::proto::TeamChatType type,
                              const std::vector<uint8_t>& payload) override
    {
        chats.push_back({team_id, peer_id, incoming, ts, type, payload});
        return true;
    }

    std::vector<ChatSample> chats;
};

class FakeGpsLoader final : public team::ui::ITeamPageGpsTrackLoader
{
  public:
    uint32_t selectedMemberId() const override { return selected_member_id; }

    bool loadTrackFile(const char* path, bool show_toast) override
    {
        load_count += 1;
        loaded_path = path ? path : "";
        last_show_toast = show_toast;
        return load_ok;
    }

    uint32_t selected_member_id = 0xFFFFFFFFu;
    bool load_ok = true;
    int load_count = 0;
    std::string loaded_path;
    bool last_show_toast = true;
};

class FakeUnreadPublisher final : public team::ui::ITeamPageUnreadPublisher
{
  public:
    void publishTeamUnread(uint32_t unread_count) override
    {
        published.push_back(unread_count);
    }

    std::vector<uint32_t> published;
};

team::TeamId testTeamId()
{
    team::TeamId id{};
    id[0] = 0x44;
    id[1] = 0x55;
    return id;
}

team::TeamEventContext eventContext(uint32_t from, uint32_t timestamp)
{
    team::TeamEventContext context;
    context.team_id = testTeamId();
    context.key_id = 7;
    context.from = from;
    context.timestamp = timestamp;
    return context;
}

team::ui::TeamPageActivityState activityState(uint32_t unread = 0)
{
    team::ui::TeamPageActivityState state;
    state.team_id = testTeamId();
    state.has_team_id = true;
    state.team_chat_unread = unread;
    return state;
}

team::ui::TeamPageActivitySink makeSink(FakePositionLog& positions,
                                        FakeChatLog& chats,
                                        FakeGpsLoader& gps,
                                        FakeUnreadPublisher& unread,
                                        bool chat_visible = false)
{
    team::ui::TeamPageActivityContext context;
    context.now_s = 1000;
    context.self_node_id = 0x11111111;
    context.team_chat_visible = chat_visible;
    return team::ui::TeamPageActivitySink(positions,
                                          chats,
                                          gps,
                                          unread,
                                          context);
}

std::vector<uint8_t> encodePosition(int32_t lat_e7,
                                    int32_t lon_e7,
                                    uint32_t ts)
{
    team::proto::TeamPositionMessage position;
    position.flags = team::proto::kTeamPosHasAltitude |
                     team::proto::kTeamPosHasSpeed;
    position.lat_e7 = lat_e7;
    position.lon_e7 = lon_e7;
    position.alt_m = 321;
    position.speed_dmps = 42;
    position.ts = ts;
    std::vector<uint8_t> payload;
    assert(team::proto::encodeTeamPositionMessage(position, payload));
    return payload;
}

std::vector<uint8_t> encodeTrack()
{
    team::proto::TeamTrackMessage track;
    track.start_ts = 0;
    track.interval_s = 10;
    track.valid_mask = 0b101u;
    track.points.push_back({100000001, 200000001});
    track.points.push_back({100000002, 200000002});
    track.points.push_back({100000003, 200000003});
    std::vector<uint8_t> payload;
    assert(team::proto::encodeTeamTrackMessage(track, payload));
    return payload;
}

std::vector<uint8_t> encodeLocationPayload(uint32_t ts)
{
    team::proto::TeamChatLocation location;
    location.lat_e7 = 333000000;
    location.lon_e7 = 444000000;
    location.alt_m = 66;
    location.ts = ts;
    location.label = "ridge";
    std::vector<uint8_t> payload;
    assert(team::proto::encodeTeamChatLocation(location, payload));
    return payload;
}

void testPositionPayloadAppendsSample()
{
    FakePositionLog positions;
    FakeChatLog chats;
    FakeGpsLoader gps;
    FakeUnreadPublisher unread;
    auto sink = makeSink(positions, chats, gps, unread);
    auto state = activityState();

    team::TeamPositionEvent event;
    event.ctx = eventContext(0x22222222, 0);
    event.payload = encodePosition(123456789, 987654321, 0);

    sink.consumePosition(state, event);

    assert(positions.positions.size() == 1);
    assert(positions.positions[0].team_id == testTeamId());
    assert(positions.positions[0].member_id == 0x22222222);
    assert(positions.positions[0].lat_e7 == 123456789);
    assert(positions.positions[0].lon_e7 == 987654321);
    assert(positions.positions[0].alt_m == 321);
    assert(positions.positions[0].speed_dmps == 42);
    assert(positions.positions[0].ts == 1000);
}

void testWaypointWithLocationAppendsSample()
{
    FakePositionLog positions;
    FakeChatLog chats;
    FakeGpsLoader gps;
    FakeUnreadPublisher unread;
    auto sink = makeSink(positions, chats, gps, unread);
    auto state = activityState();

    team::TeamWaypointEvent event;
    event.ctx = eventContext(0x22222222, 777);
    event.msg.flags = team::proto::kTeamWaypointHasLocation;
    event.msg.lat_e7 = 111;
    event.msg.lon_e7 = 222;

    sink.consumeWaypoint(state, event);

    assert(positions.positions.size() == 1);
    assert(positions.positions[0].lat_e7 == 111);
    assert(positions.positions[0].lon_e7 == 222);
    assert(positions.positions[0].ts == 777);
}

void testTrackAppendsValidPointsAndReloadsSelectedMember()
{
    FakePositionLog positions;
    FakeChatLog chats;
    FakeGpsLoader gps;
    FakeUnreadPublisher unread;
    gps.selected_member_id = 0x22222222;
    auto sink = makeSink(positions, chats, gps, unread);
    auto state = activityState();

    team::TeamTrackEvent event;
    event.ctx = eventContext(0x22222222, 500);
    event.payload = encodeTrack();

    sink.consumeTrack(state, event);

    assert(positions.positions.size() == 2);
    assert(positions.positions[0].lat_e7 == 100000001);
    assert(positions.positions[0].ts == 500);
    assert(positions.positions[1].lat_e7 == 100000003);
    assert(positions.positions[1].ts == 520);
    assert(positions.append_track_count == 1);
    assert(positions.track_member_id == 0x22222222);
    assert(positions.path_requested_count == 1);
    assert(gps.load_count == 1);
    assert(gps.loaded_path == "/tmp/member.trk");
    assert(!gps.last_show_toast);

    gps.selected_member_id = 0x33333333;
    event.ctx.from = 0x44444444;
    sink.consumeTrack(state, event);
    assert(gps.load_count == 1);
}

void testChatAppendsLogAndUpdatesUnread()
{
    FakePositionLog positions;
    FakeChatLog chats;
    FakeGpsLoader gps;
    FakeUnreadPublisher unread;
    auto sink = makeSink(positions, chats, gps, unread, false);
    auto state = activityState(4);

    team::TeamChatEvent event;
    event.ctx = eventContext(0x22222222, 900);
    event.msg.header.from = 0x22222222;
    event.msg.header.type = team::proto::TeamChatType::Text;
    event.msg.payload = {1, 2, 3};

    sink.consumeChat(state, event);

    assert(chats.chats.size() == 1);
    assert(chats.chats[0].peer_id == 0x22222222);
    assert(chats.chats[0].incoming);
    assert(chats.chats[0].ts == 900);
    assert(state.team_chat_unread == 5);
    assert(unread.published.size() == 1);
    assert(unread.published[0] == 5);

    event.msg.header.from = 0x11111111;
    sink.consumeChat(state, event);
    assert(state.team_chat_unread == 5);
    assert(unread.published.size() == 1);
}

void testVisibleChatResetsUnread()
{
    FakePositionLog positions;
    FakeChatLog chats;
    FakeGpsLoader gps;
    FakeUnreadPublisher unread;
    auto sink = makeSink(positions, chats, gps, unread, true);
    auto state = activityState(3);

    team::TeamChatEvent event;
    event.ctx = eventContext(0x22222222, 901);
    event.msg.header.from = 0x22222222;
    event.msg.header.type = team::proto::TeamChatType::Text;

    sink.consumeChat(state, event);

    assert(state.team_chat_unread == 0);
    assert(unread.published.size() == 1);
    assert(unread.published[0] == 0);
}

void testLocationChatAppendsPositionSample()
{
    FakePositionLog positions;
    FakeChatLog chats;
    FakeGpsLoader gps;
    FakeUnreadPublisher unread;
    auto sink = makeSink(positions, chats, gps, unread);
    auto state = activityState();

    team::TeamChatEvent event;
    event.ctx = eventContext(0x22222222, 600);
    event.msg.header.from = 0;
    event.msg.header.type = team::proto::TeamChatType::Location;
    event.msg.header.ts = 601;
    event.msg.payload = encodeLocationPayload(0);

    sink.consumeChat(state, event);

    assert(chats.chats.size() == 1);
    assert(chats.chats[0].peer_id == 0x22222222);
    assert(positions.positions.size() == 1);
    assert(positions.positions[0].member_id == 0x22222222);
    assert(positions.positions[0].lat_e7 == 333000000);
    assert(positions.positions[0].lon_e7 == 444000000);
    assert(positions.positions[0].alt_m == 66);
    assert(positions.positions[0].ts == 601);
}

} // namespace

int main()
{
    testPositionPayloadAppendsSample();
    testWaypointWithLocationAppendsSample();
    testTrackAppendsValidPointsAndReloadsSelectedMember();
    testChatAppendsLogAndUpdatesUnread();
    testVisibleChatResetsUnread();
    testLocationChatAppendsPositionSample();
    return 0;
}
