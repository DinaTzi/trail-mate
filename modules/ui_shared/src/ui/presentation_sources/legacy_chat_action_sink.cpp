#include "ui/presentation_sources/legacy_chat_action_sink.h"

#include "ui/presentation_sources/runtime_chat_action_sink.h"

namespace ui::presentation_sources
{

LegacyChatActionSink::LegacyChatActionSink(::chat::ChatService& chat_service)
    : chat_service_(chat_service)
{
}

ui::UiActionResult LegacyChatActionSink::selectConversation(
    ui::chat::ConversationId id)
{
    RuntimeChatActionSink sink(chat_service_);
    return sink.selectConversation(id);
}

ui::UiActionResult LegacyChatActionSink::sendMessage(
    const ui::chat::SendMessageView& message)
{
    RuntimeChatActionSink sink(chat_service_);
    return sink.sendMessage(message);
}

ui::UiActionResult LegacyChatActionSink::markRead(ui::chat::ConversationId id)
{
    RuntimeChatActionSink sink(chat_service_);
    return sink.markRead(id);
}

} // namespace ui::presentation_sources
