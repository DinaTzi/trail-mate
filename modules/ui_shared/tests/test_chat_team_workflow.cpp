#include "ui/screens/chat/chat_team_workflow.h"

#include <cassert>
#include <cstring>

namespace
{

ui::chat::ConversationId teamConversation()
{
    ui::chat::ConversationId id;
    id.kind = ui::chat::ConversationKind::Team;
    id.protocol = ui::chat::ChatProtocolKind::TrailMate;
    id.primary = 1;
    return id;
}

class FakeTeamSource final : public ui::chat::IChatPresentationSource
{
  public:
    bool buildChatWorkspaceSnapshot(
        const ui::chat::ChatWorkspaceRequest& request,
        ui::chat::ChatWorkspaceSnapshot& out) const override
    {
        ui::chat::resetChatWorkspaceSnapshot(out);
        out.header.valid = true;
        out.selected_conversation = request.selected;
        out.conversation_count = 1;
        out.conversations[0].id = teamConversation();
        out.conversations[0].kind = ui::chat::ConversationKind::Team;
        ui::copyText(out.conversations[0].title, "Trail team");
        out.conversations[0].selected =
            request.selected == out.conversations[0].id;
        if (out.conversations[0].selected)
        {
            out.message_count = 1;
            out.messages[0].conversation = out.conversations[0].id;
            ui::copyText(out.messages[0].text, "ready");
        }
        return true;
    }
};

class FakeTeamChatSink final : public ui::chat::IChatActionSink
{
  public:
    ui::UiActionResult selectConversation(ui::chat::ConversationId id) override
    {
        selected = id;
        return ui::UiActionResult::success();
    }

    ui::UiActionResult sendMessage(
        const ui::chat::SendMessageView& message) override
    {
        ++send_count;
        last_sent = message.text;
        return send_result;
    }

    ui::UiActionResult markRead(ui::chat::ConversationId id) override
    {
        marked = id;
        return ui::UiActionResult::success();
    }

    ui::chat::ConversationId selected{};
    ui::chat::ConversationId marked{};
    int send_count = 0;
    const char* last_sent = nullptr;
    ui::UiActionResult send_result = ui::UiActionResult::success();
};

class FakeTeamActionSink final : public ui::team_actions::ITeamActionSink
{
  public:
    ui::UiActionResult sendTeamAction(
        const ui::team_actions::TeamActionRequest& request) override
    {
        ++send_count;
        last_request = request;
        return result;
    }

    int send_count = 0;
    ui::team_actions::TeamActionRequest last_request{};
    ui::UiActionResult result = ui::UiActionResult::success();
};

} // namespace

int main()
{
    FakeTeamSource source;
    FakeTeamChatSink chat_sink;
    ui::chat::ChatWorkspaceModel model(source, chat_sink);
    FakeTeamActionSink action_sink;
    chat::ui::ChatTeamWorkflow workflow(model, &action_sink);

    ui::chat::ChatWorkspaceSnapshot snapshot;
    assert(workflow.buildSelectedSnapshot(snapshot));
    assert(snapshot.conversation_count == 1);
    assert(snapshot.message_count == 1);
    assert(chat_sink.selected == teamConversation());

    const auto text_result = workflow.sendText("hello team");
    assert(text_result.ok);
    assert(chat_sink.send_count == 1);
    assert(std::strcmp(chat_sink.last_sent, "hello team") == 0);

    chat_sink.send_result =
        ui::UiActionResult::fail(ui::UiActionFailure::NotReady);
    const auto failed_text = workflow.sendText("not ready");
    assert(!failed_text.ok);
    assert(std::strcmp(workflow.textSendFailureMessage(failed_text),
                       "Team keys not ready") == 0);

    const auto marker_result = workflow.sendCurrentLocationMarker(1);
    assert(marker_result.ok);
    assert(action_sink.send_count == 1);
    assert(action_sink.last_request.kind ==
           ui::team_actions::TeamActionKind::LocationMarker);
    assert(action_sink.last_request.location.use_current_location);
    assert(action_sink.last_request.location.marker_icon == 1);
    assert(action_sink.last_request.location.label != nullptr);

    const auto invalid_marker = workflow.sendCurrentLocationMarker(99);
    assert(!invalid_marker.ok);
    assert(invalid_marker.failure == ui::UiActionFailure::InvalidInput);
    assert(std::strcmp(workflow.locationSendFailureMessage(invalid_marker),
                       "Invalid marker") == 0);

    chat::ui::ChatTeamWorkflow no_action_sink(model, nullptr);
    const auto missing_sink = no_action_sink.sendCurrentLocationMarker(1);
    assert(!missing_sink.ok);
    assert(missing_sink.failure == ui::UiActionFailure::NotReady);

    assert(workflow.markRead(teamConversation()).ok);
    assert(chat_sink.marked == teamConversation());

    return 0;
}
