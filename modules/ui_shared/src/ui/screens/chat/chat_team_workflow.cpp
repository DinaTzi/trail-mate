#include "ui/screens/chat/chat_team_workflow.h"

#include "team/protocol/team_location_marker.h"

namespace chat::ui
{

ChatTeamWorkflow::ChatTeamWorkflow(
    ::ui::chat::ChatWorkspaceModel& team_chat_model,
    ::ui::team_actions::ITeamActionSink* team_action_sink)
    : team_chat_model_(team_chat_model),
      team_action_sink_(team_action_sink)
{
}

bool ChatTeamWorkflow::buildSnapshot(::ui::chat::ChatWorkspaceSnapshot& out)
{
    return team_chat_model_.buildSnapshot(out);
}

bool ChatTeamWorkflow::buildSelectedSnapshot(
    ::ui::chat::ChatWorkspaceSnapshot& out)
{
    if (!team_chat_model_.buildSnapshot(out) || out.conversation_count == 0)
    {
        return false;
    }

    (void)team_chat_model_.selectConversation(out.conversations[0].id);
    (void)team_chat_model_.buildSnapshot(out);
    return out.header.valid && out.conversation_count > 0;
}

::ui::UiActionResult ChatTeamWorkflow::markRead(
    const ::ui::chat::ConversationId& conversation)
{
    return team_chat_model_.markRead(conversation);
}

::ui::UiActionResult ChatTeamWorkflow::sendText(const char* text)
{
    return team_chat_model_.sendMessage(text);
}

::ui::UiActionResult ChatTeamWorkflow::sendCurrentLocationMarker(
    uint8_t icon_id)
{
    if (!team::proto::team_location_marker_icon_is_valid(icon_id))
    {
        return ::ui::UiActionResult::fail(::ui::UiActionFailure::InvalidInput);
    }
    if (team_action_sink_ == nullptr)
    {
        return ::ui::UiActionResult::fail(::ui::UiActionFailure::NotReady);
    }

    ::ui::team_actions::TeamActionRequest request;
    request.kind = ::ui::team_actions::TeamActionKind::LocationMarker;
    request.location.use_current_location = true;
    request.location.marker_icon = icon_id;
    request.location.label = team::proto::team_location_marker_icon_name(icon_id);

    return team_action_sink_->sendTeamAction(request);
}

const char* ChatTeamWorkflow::textSendFailureMessage(
    ::ui::UiActionResult result) const
{
    if (result.failure == ::ui::UiActionFailure::NotReady)
    {
        return "Team keys not ready";
    }
    if (result.failure == ::ui::UiActionFailure::Unsupported)
    {
        return "Team chat unsupported";
    }
    if (result.failure == ::ui::UiActionFailure::InvalidInput)
    {
        return "Message unavailable";
    }
    return "Team chat send failed";
}

const char* ChatTeamWorkflow::locationSendFailureMessage(
    ::ui::UiActionResult result) const
{
    if (result.failure == ::ui::UiActionFailure::NotReady)
    {
        return "Team location not ready";
    }
    if (result.failure == ::ui::UiActionFailure::Unsupported)
    {
        return "Team location unsupported";
    }
    if (result.failure == ::ui::UiActionFailure::InvalidInput)
    {
        return "Invalid marker";
    }
    return "Team location send failed";
}

} // namespace chat::ui
