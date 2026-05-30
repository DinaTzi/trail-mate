#pragma once

#include "ui/presentation_sources/team_chat_action_sink.h"
#include "ui/team_actions/team_action_sink.h"

#include <cstddef>
#include <cstdint>

namespace team
{
class TeamController;
} // namespace team

namespace ui::team_actions
{

class TeamControllerChatCommandPort final
    : public ::ui::presentation_sources::ITeamChatCommandPort
{
  public:
    explicit TeamControllerChatCommandPort(::team::TeamController& controller);

    bool setKeysFromPsk(const ::team::TeamId& team_id,
                        uint32_t key_id,
                        const uint8_t* psk,
                        size_t psk_len) override;
    bool sendTeamChat(const ::team::proto::TeamChatMessage& message,
                      uint8_t team_channel_raw) override;

  private:
    ::team::TeamController& controller_;
};

class GpsTeamLocationSource final : public ITeamLocationSource
{
  public:
    bool currentTeamLocation(TeamLocationSnapshot& out) override;
};

} // namespace ui::team_actions
