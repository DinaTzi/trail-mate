#include "ui/screens/team/team_page_runtime_port.h"

namespace team
{
namespace ui
{

TeamPageRuntimePort::TeamPageRuntimePort(
    ITeamPageControllerPort* controller,
    ITeamPagePairingPort* pairing,
    ITeamPageKeyStorePort* key_store)
    : controller_(controller),
      pairing_(pairing),
      key_store_(key_store)
{
}

bool TeamPageRuntimePort::hasController() const
{
    return controller_ != nullptr;
}

bool TeamPageRuntimePort::hasPairing() const
{
    return pairing_ != nullptr;
}

void TeamPageRuntimePort::clearKeys() const
{
    if (controller_)
    {
        controller_->clearKeys();
    }
}

void TeamPageRuntimePort::resetControllerUi() const
{
    if (controller_)
    {
        controller_->resetUiState();
    }
}

void TeamPageRuntimePort::stopPairing() const
{
    if (pairing_)
    {
        pairing_->stop();
    }
}

team::TeamPairingStatus TeamPageRuntimePort::pairingStatus() const
{
    if (!pairing_)
    {
        return {};
    }
    return pairing_->status();
}

bool TeamPageRuntimePort::startLeader(const TeamId& team_id,
                                      uint32_t key_id,
                                      const uint8_t* psk,
                                      size_t psk_len,
                                      uint32_t leader_id,
                                      const char* team_name) const
{
    return pairing_ && pairing_->startLeader(team_id,
                                             key_id,
                                             psk,
                                             psk_len,
                                             leader_id,
                                             team_name);
}

bool TeamPageRuntimePort::startMember(uint32_t self_id) const
{
    return pairing_ && pairing_->startMember(self_id);
}

bool TeamPageRuntimePort::setKeysFromPsk(const TeamId& team_id,
                                         uint32_t key_id,
                                         const uint8_t* psk,
                                         size_t psk_len) const
{
    return controller_ &&
           controller_->setKeysFromPsk(team_id, key_id, psk, psk_len);
}

bool TeamPageRuntimePort::saveKeysNow(
    const TeamId& team_id,
    uint32_t key_id,
    const std::array<uint8_t, team::proto::kTeamChannelPskSize>& psk) const
{
    return key_store_ && key_store_->saveKeysNow(team_id, key_id, psk);
}

bool TeamPageRuntimePort::sendKick(const team::proto::TeamKick& kick,
                                   chat::ChannelId channel,
                                   chat::NodeId dest) const
{
    return controller_ && controller_->sendKick(kick, channel, dest);
}

bool TeamPageRuntimePort::sendTransferLeader(
    const team::proto::TeamTransferLeader& transfer,
    chat::ChannelId channel,
    chat::NodeId dest) const
{
    return controller_ &&
           controller_->sendTransferLeader(transfer, channel, dest);
}

bool TeamPageRuntimePort::sendKeyDist(
    const team::proto::TeamKeyDist& key_dist,
    chat::ChannelId channel,
    chat::NodeId dest) const
{
    return controller_ && controller_->sendKeyDist(key_dist, channel, dest);
}

bool TeamPageRuntimePort::sendKeyDistPlain(
    const team::proto::TeamKeyDist& key_dist,
    chat::ChannelId channel,
    chat::NodeId dest) const
{
    return controller_ &&
           controller_->sendKeyDistPlain(key_dist, channel, dest);
}

bool TeamPageRuntimePort::sendKeyRequest(
    const team::proto::TeamKeyRequest& request,
    chat::ChannelId channel,
    chat::NodeId dest) const
{
    return controller_ &&
           controller_->sendKeyRequest(request, channel, dest);
}

bool TeamPageRuntimePort::sendStatus(const team::proto::TeamStatus& status,
                                     chat::ChannelId channel,
                                     chat::NodeId dest) const
{
    return controller_ && controller_->sendStatus(status, channel, dest);
}

bool TeamPageRuntimePort::sendStatusPlain(
    const team::proto::TeamStatus& status,
    chat::ChannelId channel,
    chat::NodeId dest) const
{
    return controller_ && controller_->sendStatusPlain(status, channel, dest);
}

team::TeamService::SendError TeamPageRuntimePort::lastSendError() const
{
    if (!controller_)
    {
        return team::TeamService::SendError::None;
    }
    return controller_->lastSendError();
}

} // namespace ui
} // namespace team
