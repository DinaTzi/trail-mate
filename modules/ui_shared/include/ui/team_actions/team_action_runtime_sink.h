#pragma once

#include "platform/ui/team_ui_chat_log_store.h"
#include "platform/ui/team_ui_snapshot_store.h"
#include "ui/presentation_sources/team_chat_action_sink.h"
#include "ui/team_actions/team_action_sink.h"

#include <stdint.h>

namespace ui::team_actions
{

// TeamActionRuntimeSink is the runtime adapter behind the Team action seam.
//
// Pattern:
//   Command Sink / Runtime Adapter.
//
// It translates presentation-level Team actions into Team runtime sends and
// local Team chat-log persistence.
//
// It must not:
//   - build ChatWorkspaceSnapshot
//   - access LVGL widgets
//   - map Team to DirectPeer/Channel
//   - use LegacyChatActionSink
//   - expose raw packet encoding to ChatUiController
class TeamActionRuntimeSink final : public ITeamActionSink
{
  public:
    TeamActionRuntimeSink(
        ::team::ui::ITeamUiSnapshotStore& snapshot_store,
        ::team::ui::ITeamUiChatLogStore& chat_log_store,
        ::ui::presentation_sources::ITeamChatCommandPort* command_port,
        ITeamLocationSource* location_source = nullptr,
        uint8_t team_channel_raw = 2);

    ui::UiActionResult sendTeamAction(
        const TeamActionRequest& request) override;

  private:
    ui::UiActionResult sendText(const TeamActionRequest& request);
    ui::UiActionResult sendLocationShare(const TeamActionRequest& request);
    ui::UiActionResult sendLocationMarker(const TeamActionRequest& request);
    ui::UiActionResult sendCommand(const TeamActionRequest& request);

    ::team::ui::ITeamUiSnapshotStore& snapshot_store_;
    ::team::ui::ITeamUiChatLogStore& chat_log_store_;
    ::ui::presentation_sources::ITeamChatCommandPort* command_port_ = nullptr;
    ITeamLocationSource* location_source_ = nullptr;
    uint8_t team_channel_raw_ = 2;
    uint32_t next_message_id_ = 1;
};

} // namespace ui::team_actions
