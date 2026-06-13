#pragma once

#include "chat/runtime/protocol_runtime.h"
#include "meshtastic/mesh.pb.h"

#include <cstdint>

namespace chat::runtime
{

enum class MeshtasticPkiResyncCause : uint8_t
{
    LocalPkiNotReady = 0,
    PeerKeyMissing,
    PeerKeyStale,
    PeerReportsUnknownPubkey,
    PeerReportsNoChannel,
};

struct MeshtasticPkiResyncInput
{
    MeshtasticPkiResyncCause cause = MeshtasticPkiResyncCause::PeerKeyMissing;
    NodeId peer = 0;
    MessageId request_id = 0;
    ChannelId channel = ChannelId::PRIMARY;
};

class MeshtasticPkiResyncState
{
  public:
    ProtocolEffects handle(const MeshtasticPkiResyncInput& input) const
    {
        ProtocolEffects effects{};
        if (input.peer == 0)
        {
            return effects;
        }

        if (input.cause == MeshtasticPkiResyncCause::PeerKeyStale)
        {
            ForgetPeerKeyEffect forget{};
            forget.protocol = MeshProtocol::Meshtastic;
            forget.peer = input.peer;
            effects.add(forget);
        }

        SendNodeInfoEffect node_info{};
        node_info.protocol = MeshProtocol::Meshtastic;
        node_info.channel = input.channel;
        node_info.peer = input.peer;
        node_info.want_response = true;
        effects.add(node_info);

        if (shouldReplyWithRoutingError(input.cause) && input.request_id != 0)
        {
            SendRoutingErrorEffect routing{};
            routing.protocol = MeshProtocol::Meshtastic;
            routing.channel = input.channel;
            routing.peer = input.peer;
            routing.request_id = input.request_id;
            routing.error_code = meshtastic_Routing_Error_PKI_UNKNOWN_PUBKEY;
            effects.add(routing);
        }

        return effects;
    }

  private:
    static bool shouldReplyWithRoutingError(MeshtasticPkiResyncCause cause)
    {
        return cause == MeshtasticPkiResyncCause::LocalPkiNotReady ||
               cause == MeshtasticPkiResyncCause::PeerKeyMissing ||
               cause == MeshtasticPkiResyncCause::PeerKeyStale;
    }
};

class MeshtasticRuntime final : public IProtocolRuntime
{
  public:
    ProtocolEffects prepareOutgoing(const ProtocolIntent&,
                                    const RuntimeContext&) override
    {
        return ProtocolEffects{};
    }

    ProtocolEffects handleIncoming(const IncomingPacket&,
                                   const RuntimeContext&) override
    {
        return ProtocolEffects{};
    }

    ProtocolEffects handleTxResult(const TxResult&,
                                   const RuntimeContext&) override
    {
        return ProtocolEffects{};
    }

    ProtocolEffects tick(const RuntimeContext&) override
    {
        return ProtocolEffects{};
    }

    ProtocolEffects handlePkiResync(const MeshtasticPkiResyncInput& input) const
    {
        return pki_resync_.handle(input);
    }

  private:
    MeshtasticPkiResyncState pki_resync_{};
};

} // namespace chat::runtime
