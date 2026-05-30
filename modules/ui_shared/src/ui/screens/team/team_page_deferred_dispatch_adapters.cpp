#include "ui/screens/team/team_page_deferred_dispatch.h"

#include "ui/screens/team/team_page_runtime_port.h"

namespace team
{
namespace ui
{

TeamPageDeferredDispatchRuntimeAdapter::TeamPageDeferredDispatchRuntimeAdapter(
    const TeamPageRuntimePort& runtime)
    : runtime_(runtime)
{
}

bool TeamPageDeferredDispatchRuntimeAdapter::hasController() const
{
    return runtime_.hasController();
}

bool TeamPageDeferredDispatchRuntimeAdapter::sendKeyDistPlain(
    const team::proto::TeamKeyDist& key_dist,
    chat::ChannelId channel,
    chat::NodeId dest)
{
    return runtime_.sendKeyDistPlain(key_dist, channel, dest);
}

bool TeamPageDeferredDispatchRuntimeAdapter::sendStatus(
    const team::proto::TeamStatus& status,
    chat::ChannelId channel,
    chat::NodeId dest)
{
    return runtime_.sendStatus(status, channel, dest);
}

bool TeamPageDeferredDispatchRuntimeAdapter::sendStatusPlain(
    const team::proto::TeamStatus& status,
    chat::ChannelId channel,
    chat::NodeId dest)
{
    return runtime_.sendStatusPlain(status, channel, dest);
}

team::TeamService::SendError
TeamPageDeferredDispatchRuntimeAdapter::lastSendError() const
{
    return runtime_.lastSendError();
}

} // namespace ui
} // namespace team
