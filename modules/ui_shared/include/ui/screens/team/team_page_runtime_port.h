#pragma once

#include "chat/domain/chat_types.h"
#include "team/domain/team_types.h"
#include "team/protocol/team_mgmt.h"
#include "team/usecase/team_pairing_service.h"
#include "team/usecase/team_service.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace team
{

class TeamController;
class TeamPairingService;

namespace ui
{

class ITeamPageControllerPort
{
  public:
    virtual ~ITeamPageControllerPort() = default;

    virtual void clearKeys() = 0;
    virtual void resetUiState() = 0;
    virtual bool setKeysFromPsk(const TeamId& team_id,
                                uint32_t key_id,
                                const uint8_t* psk,
                                size_t psk_len) = 0;
    virtual bool sendKick(const team::proto::TeamKick& kick,
                          chat::ChannelId channel,
                          chat::NodeId dest) = 0;
    virtual bool sendTransferLeader(
        const team::proto::TeamTransferLeader& transfer,
        chat::ChannelId channel,
        chat::NodeId dest) = 0;
    virtual bool sendKeyDist(const team::proto::TeamKeyDist& key_dist,
                             chat::ChannelId channel,
                             chat::NodeId dest) = 0;
    virtual bool sendKeyDistPlain(
        const team::proto::TeamKeyDist& key_dist,
        chat::ChannelId channel,
        chat::NodeId dest) = 0;
    virtual bool sendKeyRequest(
        const team::proto::TeamKeyRequest& request,
        chat::ChannelId channel,
        chat::NodeId dest) = 0;
    virtual bool sendStatus(const team::proto::TeamStatus& status,
                            chat::ChannelId channel,
                            chat::NodeId dest) = 0;
    virtual bool sendStatusPlain(const team::proto::TeamStatus& status,
                                 chat::ChannelId channel,
                                 chat::NodeId dest) = 0;
    virtual team::TeamService::SendError lastSendError() const = 0;
};

class ITeamPagePairingPort
{
  public:
    virtual ~ITeamPagePairingPort() = default;

    virtual bool startLeader(const TeamId& team_id,
                             uint32_t key_id,
                             const uint8_t* psk,
                             size_t psk_len,
                             uint32_t leader_id,
                             const char* team_name) = 0;
    virtual bool startMember(uint32_t self_id) = 0;
    virtual void stop() = 0;
    virtual team::TeamPairingStatus status() const = 0;
};

class ITeamPageKeyStorePort
{
  public:
    virtual ~ITeamPageKeyStorePort() = default;

    virtual bool saveKeysNow(
        const TeamId& team_id,
        uint32_t key_id,
        const std::array<uint8_t, team::proto::kTeamChannelPskSize>& psk) = 0;
};

class TeamPageRuntimePort
{
  public:
    TeamPageRuntimePort(ITeamPageControllerPort* controller,
                        ITeamPagePairingPort* pairing,
                        ITeamPageKeyStorePort* key_store);

    bool hasController() const;
    bool hasPairing() const;

    void clearKeys() const;
    void resetControllerUi() const;
    void stopPairing() const;
    team::TeamPairingStatus pairingStatus() const;

    bool startLeader(const TeamId& team_id,
                     uint32_t key_id,
                     const uint8_t* psk,
                     size_t psk_len,
                     uint32_t leader_id,
                     const char* team_name) const;
    bool startMember(uint32_t self_id) const;
    bool setKeysFromPsk(const TeamId& team_id,
                        uint32_t key_id,
                        const uint8_t* psk,
                        size_t psk_len) const;
    bool saveKeysNow(
        const TeamId& team_id,
        uint32_t key_id,
        const std::array<uint8_t, team::proto::kTeamChannelPskSize>& psk) const;

    bool sendKick(const team::proto::TeamKick& kick,
                  chat::ChannelId channel,
                  chat::NodeId dest = 0) const;
    bool sendTransferLeader(const team::proto::TeamTransferLeader& transfer,
                            chat::ChannelId channel,
                            chat::NodeId dest = 0) const;
    bool sendKeyDist(const team::proto::TeamKeyDist& key_dist,
                     chat::ChannelId channel,
                     chat::NodeId dest) const;
    bool sendKeyDistPlain(const team::proto::TeamKeyDist& key_dist,
                          chat::ChannelId channel,
                          chat::NodeId dest) const;
    bool sendKeyRequest(const team::proto::TeamKeyRequest& request,
                        chat::ChannelId channel,
                        chat::NodeId dest) const;
    bool sendStatus(const team::proto::TeamStatus& status,
                    chat::ChannelId channel,
                    chat::NodeId dest = 0) const;
    bool sendStatusPlain(const team::proto::TeamStatus& status,
                         chat::ChannelId channel,
                         chat::NodeId dest = 0) const;
    team::TeamService::SendError lastSendError() const;

  private:
    ITeamPageControllerPort* controller_ = nullptr;
    ITeamPagePairingPort* pairing_ = nullptr;
    ITeamPageKeyStorePort* key_store_ = nullptr;
};

class TeamPageControllerPortAdapter final : public ITeamPageControllerPort
{
  public:
    explicit TeamPageControllerPortAdapter(team::TeamController* controller);

    void clearKeys() override;
    void resetUiState() override;
    bool setKeysFromPsk(const TeamId& team_id,
                        uint32_t key_id,
                        const uint8_t* psk,
                        size_t psk_len) override;
    bool sendKick(const team::proto::TeamKick& kick,
                  chat::ChannelId channel,
                  chat::NodeId dest) override;
    bool sendTransferLeader(const team::proto::TeamTransferLeader& transfer,
                            chat::ChannelId channel,
                            chat::NodeId dest) override;
    bool sendKeyDist(const team::proto::TeamKeyDist& key_dist,
                     chat::ChannelId channel,
                     chat::NodeId dest) override;
    bool sendKeyDistPlain(const team::proto::TeamKeyDist& key_dist,
                          chat::ChannelId channel,
                          chat::NodeId dest) override;
    bool sendKeyRequest(const team::proto::TeamKeyRequest& request,
                        chat::ChannelId channel,
                        chat::NodeId dest) override;
    bool sendStatus(const team::proto::TeamStatus& status,
                    chat::ChannelId channel,
                    chat::NodeId dest) override;
    bool sendStatusPlain(const team::proto::TeamStatus& status,
                         chat::ChannelId channel,
                         chat::NodeId dest) override;
    team::TeamService::SendError lastSendError() const override;

  private:
    team::TeamController* controller_ = nullptr;
};

class TeamPagePairingPortAdapter final : public ITeamPagePairingPort
{
  public:
    explicit TeamPagePairingPortAdapter(team::TeamPairingService* pairing);

    bool startLeader(const TeamId& team_id,
                     uint32_t key_id,
                     const uint8_t* psk,
                     size_t psk_len,
                     uint32_t leader_id,
                     const char* team_name) override;
    bool startMember(uint32_t self_id) override;
    void stop() override;
    team::TeamPairingStatus status() const override;

  private:
    team::TeamPairingService* pairing_ = nullptr;
};

class TeamPageKeyStorePortAdapter final : public ITeamPageKeyStorePort
{
  public:
    bool saveKeysNow(
        const TeamId& team_id,
        uint32_t key_id,
        const std::array<uint8_t, team::proto::kTeamChannelPskSize>& psk) override;
};

} // namespace ui
} // namespace team
