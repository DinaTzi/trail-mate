#include "ui/team_actions/team_runtime_adapters.h"

#include "platform/ui/gps_runtime.h"
#include "team/usecase/team_controller.h"

namespace ui::team_actions
{

TeamControllerChatCommandPort::TeamControllerChatCommandPort(
    ::team::TeamController& controller)
    : controller_(controller)
{
}

bool TeamControllerChatCommandPort::setKeysFromPsk(
    const ::team::TeamId& team_id,
    uint32_t key_id,
    const uint8_t* psk,
    size_t psk_len)
{
    return controller_.setKeysFromPsk(team_id, key_id, psk, psk_len);
}

bool TeamControllerChatCommandPort::sendTeamChat(
    const ::team::proto::TeamChatMessage& message,
    uint8_t team_channel_raw)
{
    return controller_.onChat(
        message,
        static_cast<::chat::ChannelId>(team_channel_raw));
}

bool GpsTeamLocationSource::currentTeamLocation(TeamLocationSnapshot& out)
{
    const auto gps_state = platform::ui::gps::get_data();
    if (!gps_state.valid)
    {
        out = {};
        return false;
    }

    out.valid = true;
    out.lat = gps_state.lat;
    out.lon = gps_state.lng;
    out.has_altitude = gps_state.has_alt;
    out.altitude_m = gps_state.alt_m;
    out.accuracy_m = 0.0f;
    out.timestamp = 0;
    return true;
}

} // namespace ui::team_actions
