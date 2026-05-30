#include "ui/screens/team/team_page_key_request_action.h"

#include "chat/domain/chat_types.h"

namespace team
{
namespace ui
{
namespace
{

TeamPageKeyRequestFailure makeFailure(
    TeamPageKeyRequestFailureKind kind,
    team::TeamService::SendError error = team::TeamService::SendError::None)
{
    TeamPageKeyRequestFailure failure;
    failure.kind = kind;
    failure.error = error;
    return failure;
}

} // namespace

TeamPageKeyRequestEffects TeamPageKeyRequestAction::handleRequest(
    const TeamPageCommandState& state,
    const team::TeamKeyRequestEvent& event,
    const TeamPageRuntimePort& runtime) const
{
    TeamPageKeyRequestEffects effects;
    const uint32_t requester =
        event.msg.requester_id != 0 ? event.msg.requester_id : event.ctx.from;

    if (!state.in_team || !state.self_is_leader || !state.has_team_id ||
        !state.has_team_psk || requester == 0 ||
        event.msg.team_id != state.team_id)
    {
        effects.failures.push_back(
            makeFailure(TeamPageKeyRequestFailureKind::Ignored));
        return effects;
    }

    effects.accepted = true;
    if (!runtime.hasController())
    {
        effects.failures.push_back(
            makeFailure(TeamPageKeyRequestFailureKind::SendFailedDetail));
        return effects;
    }

    team::proto::TeamKeyDist key_dist{};
    key_dist.team_id = state.team_id;
    key_dist.key_id = state.security_round;
    key_dist.channel_psk_len =
        static_cast<uint8_t>(state.team_psk.size());
    key_dist.channel_psk = state.team_psk;

    effects.sent_keydist =
        runtime.sendKeyDist(key_dist, chat::ChannelId::PRIMARY, requester);
    if (!effects.sent_keydist)
    {
        effects.failures.push_back(makeFailure(
            TeamPageKeyRequestFailureKind::SendFailedDetail,
            runtime.lastSendError()));
    }
    return effects;
}

} // namespace ui
} // namespace team
