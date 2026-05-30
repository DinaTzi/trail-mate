#pragma once

#include "team/domain/team_events.h"
#include "team/protocol/team_chat.h"
#include "team/protocol/team_track.h"

#include <cstdint>
#include <string>
#include <vector>

namespace team
{
namespace ui
{

class ITeamPagePositionLog
{
  public:
    virtual ~ITeamPagePositionLog() = default;

    virtual bool appendPosition(const TeamId& team_id,
                                uint32_t member_id,
                                int32_t lat_e7,
                                int32_t lon_e7,
                                int16_t alt_m,
                                uint16_t speed_dmps,
                                uint32_t ts) = 0;
    virtual bool appendMemberTrack(
        const TeamId& team_id,
        uint32_t member_id,
        const team::proto::TeamTrackMessage& track) = 0;
    virtual bool memberTrackPath(const TeamId& team_id,
                                 uint32_t member_id,
                                 std::string& out_path) = 0;
};

class ITeamPageChatLog
{
  public:
    virtual ~ITeamPageChatLog() = default;

    virtual bool appendStructuredChat(
        const TeamId& team_id,
        uint32_t peer_id,
        bool incoming,
        uint32_t ts,
        team::proto::TeamChatType type,
        const std::vector<uint8_t>& payload) = 0;
};

class ITeamPageGpsTrackLoader
{
  public:
    virtual ~ITeamPageGpsTrackLoader() = default;

    virtual uint32_t selectedMemberId() const = 0;
    virtual bool loadTrackFile(const char* path, bool show_toast) = 0;
};

class ITeamPageUnreadPublisher
{
  public:
    virtual ~ITeamPageUnreadPublisher() = default;

    virtual void publishTeamUnread(uint32_t unread_count) = 0;
};

struct TeamPageActivityState
{
    TeamId team_id{};
    bool has_team_id = false;
    uint32_t team_chat_unread = 0;
};

struct TeamPageActivityContext
{
    uint32_t now_s = 0;
    uint32_t self_node_id = 0;
    bool team_chat_visible = false;
};

class TeamPageActivitySink
{
  public:
    TeamPageActivitySink(ITeamPagePositionLog& position_log,
                         ITeamPageChatLog& chat_log,
                         ITeamPageGpsTrackLoader& gps_track_loader,
                         ITeamPageUnreadPublisher& unread_publisher,
                         TeamPageActivityContext context);

    void consumePosition(TeamPageActivityState& state,
                         const team::TeamPositionEvent& event) const;
    void consumeWaypoint(TeamPageActivityState& state,
                         const team::TeamWaypointEvent& event) const;
    void consumeTrack(TeamPageActivityState& state,
                      const team::TeamTrackEvent& event) const;
    void consumeChat(TeamPageActivityState& state,
                     const team::TeamChatEvent& event) const;

  private:
    uint32_t fallbackTimestamp(uint32_t preferred_ts,
                               uint32_t event_ts) const;
    bool appendPositionSample(const TeamPageActivityState& state,
                              uint32_t member_id,
                              int32_t lat_e7,
                              int32_t lon_e7,
                              int16_t alt_m,
                              uint16_t speed_dmps,
                              uint32_t ts) const;
    void publishUnreadIfChanged(uint32_t previous,
                                uint32_t current) const;

    ITeamPagePositionLog& position_log_;
    ITeamPageChatLog& chat_log_;
    ITeamPageGpsTrackLoader& gps_track_loader_;
    ITeamPageUnreadPublisher& unread_publisher_;
    TeamPageActivityContext context_;
};

class TeamPageActivityStoreAdapter final : public ITeamPagePositionLog,
                                           public ITeamPageChatLog
{
  public:
    bool appendPosition(const TeamId& team_id,
                        uint32_t member_id,
                        int32_t lat_e7,
                        int32_t lon_e7,
                        int16_t alt_m,
                        uint16_t speed_dmps,
                        uint32_t ts) override;
    bool appendMemberTrack(
        const TeamId& team_id,
        uint32_t member_id,
        const team::proto::TeamTrackMessage& track) override;
    bool memberTrackPath(const TeamId& team_id,
                         uint32_t member_id,
                         std::string& out_path) override;
    bool appendStructuredChat(
        const TeamId& team_id,
        uint32_t peer_id,
        bool incoming,
        uint32_t ts,
        team::proto::TeamChatType type,
        const std::vector<uint8_t>& payload) override;
};

class TeamPageGpsTrackLoaderAdapter final : public ITeamPageGpsTrackLoader
{
  public:
    uint32_t selectedMemberId() const override;
    bool loadTrackFile(const char* path, bool show_toast) override;
};

class TeamPageUnreadPublisherAdapter final : public ITeamPageUnreadPublisher
{
  public:
    void publishTeamUnread(uint32_t unread_count) override;
};

} // namespace ui
} // namespace team
