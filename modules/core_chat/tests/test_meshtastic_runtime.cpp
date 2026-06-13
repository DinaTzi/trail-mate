#include "chat/runtime/meshtastic_runtime.h"

#include "pb_decode.h"

#include <cassert>

namespace
{

template <typename T>
const T* effectAt(const chat::runtime::ProtocolEffects& effects, size_t index)
{
    assert(index < effects.items.size());
    return std::get_if<T>(&effects.items[index]);
}

} // namespace

int main()
{
    using chat::ChannelId;
    using chat::MeshProtocol;
    using chat::runtime::EmitActionResultEffect;
    using chat::runtime::ExchangePositionIntent;
    using chat::runtime::ForgetPeerKeyEffect;
    using chat::runtime::MeshtasticPkiResyncCause;
    using chat::runtime::MeshtasticPkiResyncInput;
    using chat::runtime::MeshtasticRuntime;
    using chat::runtime::ProtocolActionKind;
    using chat::runtime::ProtocolActionState;
    using chat::runtime::RuntimeContext;
    using chat::runtime::SendNodeInfoEffect;
    using chat::runtime::SendPacketEffect;
    using chat::runtime::SendRoutingErrorEffect;
    using chat::runtime::TraceRouteIntent;

    MeshtasticRuntime runtime;
    RuntimeContext context{};
    context.protocol = MeshProtocol::Meshtastic;
    context.self_node = 0x11111111UL;
    context.now_ms = 0x20240614UL;

    {
        TraceRouteIntent intent{};
        intent.channel = ChannelId::SECONDARY;
        intent.peer = 0x22222222UL;
        intent.request_id = 0x01020304UL;
        intent.timeout_ms = 9000;

        const auto effects = runtime.prepareOutgoing(intent, context);
        assert(effects.items.size() == 1);
        const auto* packet = effectAt<SendPacketEffect>(effects, 0);
        assert(packet);
        assert(packet->protocol == MeshProtocol::Meshtastic);
        assert(packet->channel == intent.channel);
        assert(packet->dest == intent.peer);
        assert(packet->portnum == meshtastic_PortNum_TRACEROUTE_APP);
        assert(packet->request_id == intent.request_id);
        assert(packet->want_ack);
        assert(packet->want_response);

        meshtastic_RouteDiscovery decoded = meshtastic_RouteDiscovery_init_zero;
        pb_istream_t stream = pb_istream_from_buffer(packet->payload.data(),
                                                     packet->payload.size());
        assert(pb_decode(&stream, meshtastic_RouteDiscovery_fields, &decoded));
    }

    {
        ExchangePositionIntent intent{};
        intent.channel = ChannelId::PRIMARY;
        intent.peer = 0x33333333UL;
        intent.request_id = 0x05060708UL;

        const auto effects = runtime.prepareOutgoing(intent, context);
        assert(effects.items.size() == 1);
        const auto* packet = effectAt<SendPacketEffect>(effects, 0);
        assert(packet);
        assert(packet->protocol == MeshProtocol::Meshtastic);
        assert(packet->channel == intent.channel);
        assert(packet->dest == intent.peer);
        assert(packet->portnum == meshtastic_PortNum_POSITION_APP);
        assert(packet->request_id == intent.request_id);
        assert(!packet->want_ack);
        assert(packet->want_response);
        assert(packet->payload.empty());
    }

    {
        TraceRouteIntent intent{};
        intent.peer = context.self_node;
        intent.request_id = 0x0A0B0C0DUL;

        const auto effects = runtime.prepareOutgoing(intent, context);
        assert(effects.items.size() == 1);
        const auto* failed = effectAt<EmitActionResultEffect>(effects, 0);
        assert(failed);
        assert(failed->protocol == MeshProtocol::Meshtastic);
        assert(failed->action == ProtocolActionKind::TraceRoute);
        assert(failed->state == ProtocolActionState::Failed);
        assert(failed->peer == context.self_node);
        assert(failed->request_id == intent.request_id);
    }

    {
        MeshtasticPkiResyncInput input{};
        input.cause = MeshtasticPkiResyncCause::PeerKeyMissing;
        input.peer = 0xAABBCCDDUL;
        input.request_id = 0x1001UL;
        input.channel = ChannelId::PRIMARY;

        const auto effects = runtime.handlePkiResync(input);
        assert(effects.items.size() == 2);

        const auto* node_info = effectAt<SendNodeInfoEffect>(effects, 0);
        assert(node_info);
        assert(node_info->protocol == MeshProtocol::Meshtastic);
        assert(node_info->peer == input.peer);
        assert(node_info->want_response);

        const auto* routing = effectAt<SendRoutingErrorEffect>(effects, 1);
        assert(routing);
        assert(routing->peer == input.peer);
        assert(routing->request_id == input.request_id);
        assert(routing->error_code == meshtastic_Routing_Error_PKI_UNKNOWN_PUBKEY);
    }

    {
        MeshtasticPkiResyncInput input{};
        input.cause = MeshtasticPkiResyncCause::PeerKeyStale;
        input.peer = 0x01020304UL;
        input.request_id = 0x2002UL;
        input.channel = ChannelId::SECONDARY;

        const auto effects = runtime.handlePkiResync(input);
        assert(effects.items.size() == 3);

        const auto* forget = effectAt<ForgetPeerKeyEffect>(effects, 0);
        assert(forget);
        assert(forget->peer == input.peer);

        const auto* node_info = effectAt<SendNodeInfoEffect>(effects, 1);
        assert(node_info);
        assert(node_info->channel == ChannelId::SECONDARY);

        const auto* routing = effectAt<SendRoutingErrorEffect>(effects, 2);
        assert(routing);
        assert(routing->channel == ChannelId::SECONDARY);
    }

    {
        MeshtasticPkiResyncInput input{};
        input.cause = MeshtasticPkiResyncCause::PeerReportsUnknownPubkey;
        input.peer = 0x0BADF00DUL;
        input.request_id = 0x3003UL;

        const auto effects = runtime.handlePkiResync(input);
        assert(effects.items.size() == 1);
        assert(effectAt<SendNodeInfoEffect>(effects, 0));
    }

    {
        MeshtasticPkiResyncInput input{};
        input.cause = MeshtasticPkiResyncCause::LocalNoChannel;
        input.peer = 0x11223344UL;
        input.request_id = 0x5005UL;

        const auto effects = runtime.handlePkiResync(input);
        assert(effects.items.size() == 2);
        assert(effectAt<SendNodeInfoEffect>(effects, 0));

        const auto* routing = effectAt<SendRoutingErrorEffect>(effects, 1);
        assert(routing);
        assert(routing->peer == input.peer);
        assert(routing->request_id == input.request_id);
        assert(routing->error_code == meshtastic_Routing_Error_NO_CHANNEL);
    }

    {
        MeshtasticPkiResyncInput input{};
        input.cause = MeshtasticPkiResyncCause::PeerKeyMissing;
        input.peer = 0;
        input.request_id = 0x4004UL;

        const auto effects = runtime.handlePkiResync(input);
        assert(effects.empty());
    }

    return 0;
}
