#include "ui/screens/team/team_page_runtime_port.h"

#include "platform/ui/team_ui_store_runtime.h"
#include "team/usecase/team_controller.h"

namespace team
{
namespace ui
{

TeamPageControllerPortAdapter::TeamPageControllerPortAdapter(
    team::TeamController* controller)
    : controller_(controller)
{
}

void TeamPageControllerPortAdapter::clearKeys()
{
    if (controller_)
    {
        controller_->clearKeys();
    }
}

void TeamPageControllerPortAdapter::resetUiState()
{
    if (controller_)
    {
        controller_->resetUiState();
    }
}

bool TeamPageControllerPortAdapter::setKeysFromPsk(const TeamId& team_id,
                                                   uint32_t key_id,
                                                   const uint8_t* psk,
                                                   size_t psk_len)
{
    return controller_ &&
           controller_->setKeysFromPsk(team_id, key_id, psk, psk_len);
}

bool TeamPageControllerPortAdapter::sendKick(
    const team::proto::TeamKick& kick,
    chat::ChannelId channel,
    chat::NodeId dest)
{
    return controller_ && controller_->onKick(kick, channel, dest);
}

bool TeamPageControllerPortAdapter::sendTransferLeader(
    const team::proto::TeamTransferLeader& transfer,
    chat::ChannelId channel,
    chat::NodeId dest)
{
    return controller_ &&
           controller_->onTransferLeader(transfer, channel, dest);
}

bool TeamPageControllerPortAdapter::sendKeyDist(
    const team::proto::TeamKeyDist& key_dist,
    chat::ChannelId channel,
    chat::NodeId dest)
{
    return controller_ && controller_->onKeyDist(key_dist, channel, dest);
}

bool TeamPageControllerPortAdapter::sendKeyDistPlain(
    const team::proto::TeamKeyDist& key_dist,
    chat::ChannelId channel,
    chat::NodeId dest)
{
    return controller_ &&
           controller_->onKeyDistPlain(key_dist, channel, dest);
}

bool TeamPageControllerPortAdapter::sendKeyRequest(
    const team::proto::TeamKeyRequest& request,
    chat::ChannelId channel,
    chat::NodeId dest)
{
    return controller_ && controller_->onKeyRequest(request, channel, dest);
}

bool TeamPageControllerPortAdapter::sendStatus(
    const team::proto::TeamStatus& status,
    chat::ChannelId channel,
    chat::NodeId dest)
{
    return controller_ && controller_->onStatus(status, channel, dest);
}

bool TeamPageControllerPortAdapter::sendStatusPlain(
    const team::proto::TeamStatus& status,
    chat::ChannelId channel,
    chat::NodeId dest)
{
    return controller_ && controller_->onStatusPlain(status, channel, dest);
}

team::TeamService::SendError TeamPageControllerPortAdapter::lastSendError() const
{
    if (!controller_)
    {
        return team::TeamService::SendError::None;
    }
    return controller_->getLastSendError();
}

TeamPagePairingPortAdapter::TeamPagePairingPortAdapter(
    team::TeamPairingService* pairing)
    : pairing_(pairing)
{
}

bool TeamPagePairingPortAdapter::startLeader(const TeamId& team_id,
                                             uint32_t key_id,
                                             const uint8_t* psk,
                                             size_t psk_len,
                                             uint32_t leader_id,
                                             const char* team_name)
{
    return pairing_ && pairing_->startLeader(team_id,
                                             key_id,
                                             psk,
                                             psk_len,
                                             leader_id,
                                             team_name);
}

bool TeamPagePairingPortAdapter::startMember(uint32_t self_id)
{
    return pairing_ && pairing_->startMember(self_id);
}

void TeamPagePairingPortAdapter::stop()
{
    if (pairing_)
    {
        pairing_->stop();
    }
}

team::TeamPairingStatus TeamPagePairingPortAdapter::status() const
{
    if (!pairing_)
    {
        return {};
    }
    return pairing_->getStatus();
}

bool TeamPageKeyStorePortAdapter::saveKeysNow(
    const TeamId& team_id,
    uint32_t key_id,
    const std::array<uint8_t, team::proto::kTeamChannelPskSize>& psk)
{
    return team_ui_save_keys_now(team_id, key_id, psk);
}

} // namespace ui
} // namespace team
