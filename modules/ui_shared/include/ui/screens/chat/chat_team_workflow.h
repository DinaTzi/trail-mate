#pragma once

#include "ui/team_actions/team_action_sink.h"
#include "ui_presentation/chat/chat_workspace_model.h"
#include "ui_presentation/common/ui_action_result.h"

#include <stdint.h>

namespace chat::ui
{

class ChatTeamWorkflow
{
  public:
    ChatTeamWorkflow(::ui::chat::ChatWorkspaceModel& team_chat_model,
                     ::ui::team_actions::ITeamActionSink* team_action_sink);

    bool buildSnapshot(::ui::chat::ChatWorkspaceSnapshot& out);
    bool buildSelectedSnapshot(::ui::chat::ChatWorkspaceSnapshot& out);
    ::ui::UiActionResult markRead(const ::ui::chat::ConversationId& conversation);
    ::ui::UiActionResult sendText(const char* text);
    ::ui::UiActionResult sendCurrentLocationMarker(uint8_t icon_id);

    const char* textSendFailureMessage(::ui::UiActionResult result) const;
    const char* locationSendFailureMessage(::ui::UiActionResult result) const;

  private:
    ::ui::chat::ChatWorkspaceModel& team_chat_model_;
    ::ui::team_actions::ITeamActionSink* team_action_sink_ = nullptr;
};

} // namespace chat::ui
