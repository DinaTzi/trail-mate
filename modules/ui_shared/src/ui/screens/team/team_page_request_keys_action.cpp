#include "ui/screens/team/team_page_request_keys_action.h"

#include "chat/domain/chat_types.h"

namespace team
{
namespace ui
{

TeamPageRequestKeysEffects TeamPageRequestKeysAction::requestKeys(
    const TeamPageCommandState& state,
    const TeamPageRuntimePort& runtime,
    uint32_t self_node_id) const
{
    TeamPageRequestKeysEffects effects;
    if (!state.in_team || state.self_is_leader || !state.has_team_id ||
        self_node_id == 0)
    {
        effects.ignored = true;
        return effects;
    }

    effects.accepted = true;
    if (!runtime.hasController())
    {
        effects.send_failed = true;
        return effects;
    }

    team::proto::TeamKeyRequest request{};
    request.team_id = state.team_id;
    request.current_key_id = state.security_round;
    request.requester_id = self_node_id;

    effects.sent_request =
        runtime.sendKeyRequest(request, chat::ChannelId::PRIMARY, 0);
    if (!effects.sent_request)
    {
        effects.send_failed = true;
        effects.error = runtime.lastSendError();
    }
    return effects;
}

} // namespace ui
} // namespace team
