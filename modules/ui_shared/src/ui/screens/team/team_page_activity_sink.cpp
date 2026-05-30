#include "ui/screens/team/team_page_activity_sink.h"

#include "team/protocol/team_position.h"
#include "team/protocol/team_waypoint.h"

#include <cstddef>
#include <limits>

namespace team
{
namespace ui
{

TeamPageActivitySink::TeamPageActivitySink(
    ITeamPagePositionLog& position_log,
    ITeamPageChatLog& chat_log,
    ITeamPageGpsTrackLoader& gps_track_loader,
    ITeamPageUnreadPublisher& unread_publisher,
    TeamPageActivityContext context)
    : position_log_(position_log),
      chat_log_(chat_log),
      gps_track_loader_(gps_track_loader),
      unread_publisher_(unread_publisher),
      context_(context)
{
}

void TeamPageActivitySink::consumePosition(
    TeamPageActivityState& state,
    const team::TeamPositionEvent& event) const
{
    if (!state.has_team_id || event.payload.empty())
    {
        return;
    }

    team::proto::TeamPositionMessage position;
    if (!team::proto::decodeTeamPositionMessage(event.payload.data(),
                                                event.payload.size(),
                                                &position))
    {
        return;
    }

    const int16_t alt_m =
        team::proto::teamPositionHasAltitude(position) ? position.alt_m : 0;
    const uint16_t speed_dmps =
        team::proto::teamPositionHasSpeed(position) ? position.speed_dmps : 0;
    appendPositionSample(state,
                         event.ctx.from,
                         position.lat_e7,
                         position.lon_e7,
                         alt_m,
                         speed_dmps,
                         fallbackTimestamp(position.ts, event.ctx.timestamp));
}

void TeamPageActivitySink::consumeWaypoint(
    TeamPageActivityState& state,
    const team::TeamWaypointEvent& event) const
{
    if (!state.has_team_id ||
        !team::proto::teamWaypointHasLocation(event.msg))
    {
        return;
    }

    appendPositionSample(state,
                         event.ctx.from,
                         event.msg.lat_e7,
                         event.msg.lon_e7,
                         0,
                         0,
                         fallbackTimestamp(0, event.ctx.timestamp));
}

void TeamPageActivitySink::consumeTrack(
    TeamPageActivityState& state,
    const team::TeamTrackEvent& event) const
{
    if (!state.has_team_id || event.payload.empty())
    {
        return;
    }

    team::proto::TeamTrackMessage track;
    if (!team::proto::decodeTeamTrackMessage(event.payload.data(),
                                             event.payload.size(),
                                             &track) ||
        track.version != team::proto::kTeamTrackVersion)
    {
        return;
    }

    const uint32_t base_ts =
        fallbackTimestamp(track.start_ts, event.ctx.timestamp);
    for (size_t i = 0; i < track.points.size(); ++i)
    {
        if ((track.valid_mask & (1u << static_cast<uint32_t>(i))) == 0)
        {
            continue;
        }

        const auto& point = track.points[i];
        const uint32_t ts =
            base_ts + static_cast<uint32_t>(track.interval_s) *
                          static_cast<uint32_t>(i);
        appendPositionSample(state,
                             event.ctx.from,
                             point.lat_e7,
                             point.lon_e7,
                             0,
                             0,
                             ts);
    }

    position_log_.appendMemberTrack(state.team_id, event.ctx.from, track);
    if (gps_track_loader_.selectedMemberId() != event.ctx.from)
    {
        return;
    }

    std::string track_path;
    if (position_log_.memberTrackPath(state.team_id,
                                      event.ctx.from,
                                      track_path))
    {
        gps_track_loader_.loadTrackFile(track_path.c_str(), false);
    }
}

void TeamPageActivitySink::consumeChat(
    TeamPageActivityState& state,
    const team::TeamChatEvent& event) const
{
    if (!state.has_team_id)
    {
        return;
    }

    const uint32_t from_id =
        event.msg.header.from != 0 ? event.msg.header.from : event.ctx.from;
    const uint32_t ts =
        fallbackTimestamp(event.msg.header.ts, event.ctx.timestamp);

    chat_log_.appendStructuredChat(state.team_id,
                                   from_id,
                                   true,
                                   ts,
                                   event.msg.header.type,
                                   event.msg.payload);

    const uint32_t previous_unread = state.team_chat_unread;
    const bool incoming =
        from_id != 0 && from_id != context_.self_node_id;
    if (incoming && !context_.team_chat_visible)
    {
        if (state.team_chat_unread < std::numeric_limits<uint32_t>::max())
        {
            state.team_chat_unread += 1;
        }
    }
    else if (context_.team_chat_visible)
    {
        state.team_chat_unread = 0;
    }
    publishUnreadIfChanged(previous_unread, state.team_chat_unread);

    if (event.msg.header.type != team::proto::TeamChatType::Location)
    {
        return;
    }

    team::proto::TeamChatLocation location;
    if (!team::proto::decodeTeamChatLocation(event.msg.payload.data(),
                                             event.msg.payload.size(),
                                             &location))
    {
        return;
    }

    appendPositionSample(state,
                         from_id,
                         location.lat_e7,
                         location.lon_e7,
                         location.alt_m,
                         0,
                         fallbackTimestamp(location.ts, ts));
}

uint32_t TeamPageActivitySink::fallbackTimestamp(uint32_t preferred_ts,
                                                 uint32_t event_ts) const
{
    if (preferred_ts != 0)
    {
        return preferred_ts;
    }
    if (event_ts != 0)
    {
        return event_ts;
    }
    return context_.now_s;
}

bool TeamPageActivitySink::appendPositionSample(
    const TeamPageActivityState& state,
    uint32_t member_id,
    int32_t lat_e7,
    int32_t lon_e7,
    int16_t alt_m,
    uint16_t speed_dmps,
    uint32_t ts) const
{
    if (!state.has_team_id)
    {
        return false;
    }
    return position_log_.appendPosition(state.team_id,
                                        member_id,
                                        lat_e7,
                                        lon_e7,
                                        alt_m,
                                        speed_dmps,
                                        ts);
}

void TeamPageActivitySink::publishUnreadIfChanged(uint32_t previous,
                                                  uint32_t current) const
{
    if (current != previous)
    {
        unread_publisher_.publishTeamUnread(current);
    }
}

} // namespace ui
} // namespace team
