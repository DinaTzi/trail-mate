#include "chat/runtime/meshcore_runtime.h"

#include <cassert>
#include <cstring>

template <typename T>
const T* effectAt(const chat::runtime::ProtocolEffects& effects, size_t index)
{
    if (index >= effects.items.size())
    {
        return nullptr;
    }
    return std::get_if<T>(&effects.items[index]);
}

int main()
{
    using chat::MeshProtocol;
    using chat::runtime::IncomingPacket;
    using chat::runtime::EmitActionResultEffect;
    using chat::runtime::MeshCoreAppAckRegistration;
    using chat::runtime::MeshCoreRuntime;
    using chat::runtime::PublishNodeInfoEffect;
    using chat::runtime::ProtocolActionKind;
    using chat::runtime::ProtocolActionState;
    using chat::runtime::DiscoverIntent;
    using chat::runtime::RequestNodeInfoIntent;
    using chat::runtime::RuntimeContext;
    using chat::runtime::SendDiscoverRequestEffect;
    using chat::runtime::SendNodeInfoEffect;
    using chat::runtime::SendSelfAnnouncementEffect;
    using chat::runtime::SendTraceRouteEffect;
    using chat::runtime::TraceRouteIntent;
    using chat::runtime::TxResult;

    MeshCoreRuntime runtime{};
    RuntimeContext context{};
    context.protocol = MeshProtocol::MeshCore;
    context.self_node = 0x11111111UL;

    {
        DiscoverIntent intent{};
        intent.action = chat::MeshDiscoveryAction::ScanLocal;
        intent.tag = 0x13572468UL;
        intent.type_filter = 0x03;
        intent.prefix_only = true;
        intent.since = 1781258481UL;
        intent.rx_guard_ms = 7000;

        const auto effects = runtime.prepareOutgoing(intent, context);
        assert(effects.items.size() == 1);
        const auto* discover = effectAt<SendDiscoverRequestEffect>(effects, 0);
        assert(discover);
        assert(discover->protocol == MeshProtocol::MeshCore);
        assert(discover->tag == intent.tag);
        assert(discover->type_filter == intent.type_filter);
        assert(discover->prefix_only);
        assert(discover->since == intent.since);
        assert(discover->rx_guard_ms == intent.rx_guard_ms);
    }

    {
        DiscoverIntent intent{};
        intent.action = chat::MeshDiscoveryAction::SendIdLocal;
        const auto effects = runtime.prepareOutgoing(intent, context);
        assert(effects.items.size() == 1);
        const auto* announce = effectAt<SendSelfAnnouncementEffect>(effects, 0);
        assert(announce);
        assert(announce->protocol == MeshProtocol::MeshCore);
        assert(!announce->broadcast);
    }

    {
        DiscoverIntent intent{};
        intent.action = chat::MeshDiscoveryAction::SendIdBroadcast;
        const auto effects = runtime.prepareOutgoing(intent, context);
        assert(effects.items.size() == 1);
        const auto* announce = effectAt<SendSelfAnnouncementEffect>(effects, 0);
        assert(announce);
        assert(announce->protocol == MeshProtocol::MeshCore);
        assert(announce->broadcast);
    }

    {
        RequestNodeInfoIntent intent{};
        intent.peer = 0xFFFFFFFFUL;
        intent.want_response = true;
        const auto effects = runtime.prepareOutgoing(intent, context);
        assert(effects.items.size() == 1);
        const auto* node_info = effectAt<SendNodeInfoEffect>(effects, 0);
        assert(node_info);
        assert(node_info->protocol == MeshProtocol::MeshCore);
        assert(node_info->peer == 0);
        assert(node_info->want_response);
    }

    {
        uint8_t payload[chat::meshcore::kMeshCoreNodeInfoInfoPayloadSize] = {};
        size_t payload_len = 0;
        assert(chat::meshcore::buildNodeInfoQueryControlPayload(true,
                                                                payload,
                                                                sizeof(payload),
                                                                &payload_len));

        IncomingPacket packet{};
        packet.protocol = MeshProtocol::MeshCore;
        packet.portnum = chat::meshcore::kMeshCoreNodeInfoPortnum;
        packet.from = 0x22222222UL;
        packet.payload.assign(payload, payload + payload_len);

        const auto effects = runtime.handleIncoming(packet, context);
        assert(effects.items.size() == 1);
        const auto* reply = effectAt<SendNodeInfoEffect>(effects, 0);
        assert(reply);
        assert(reply->protocol == MeshProtocol::MeshCore);
        assert(reply->peer == packet.from);
        assert(!reply->want_response);
    }

    {
        chat::meshcore::MeshCoreNodeInfoBuildInfo info{};
        info.role = 2;
        info.hops = 9;
        info.node_id = 0x33333333UL;
        info.timestamp = 1781258481UL;
        info.short_name = "mc-node";
        info.long_name = "MeshCore Node";

        uint8_t payload[chat::meshcore::kMeshCoreNodeInfoInfoPayloadSize] = {};
        size_t payload_len = 0;
        assert(chat::meshcore::buildNodeInfoInfoControlPayload(info,
                                                               payload,
                                                               sizeof(payload),
                                                               &payload_len));

        IncomingPacket packet{};
        packet.protocol = MeshProtocol::MeshCore;
        packet.portnum = chat::meshcore::kMeshCoreNodeInfoPortnum;
        packet.from = 0x22222222UL;
        packet.rx_meta.hop_count = 3;
        packet.payload.assign(payload, payload + payload_len);

        const auto effects = runtime.handleIncoming(packet, context);
        assert(effects.items.size() == 1);
        const auto* publish = effectAt<PublishNodeInfoEffect>(effects, 0);
        assert(publish);
        assert(publish->protocol == MeshProtocol::MeshCore);
        assert(publish->node_id == info.node_id);
        assert(publish->role == info.role);
        assert(publish->hops == packet.rx_meta.hop_count);
        assert(publish->timestamp == info.timestamp);
        assert(publish->short_name == "mc-node");
        assert(publish->long_name == "MeshCore Node");
    }

    {
        TraceRouteIntent intent{};
        intent.peer = 0x22222222UL;
        intent.request_id = 0x01020304UL;
        intent.timeout_ms = 1000;
        context.now_ms = 5000;

        const auto effects = runtime.prepareOutgoing(intent, context);
        assert(effects.items.size() == 1);
        const auto* trace = effectAt<SendTraceRouteEffect>(effects, 0);
        assert(trace);
        assert(trace->protocol == MeshProtocol::MeshCore);
        assert(trace->peer == intent.peer);
        assert(trace->request_id == intent.request_id);
        assert(trace->timeout_ms == intent.timeout_ms);

        TxResult tx{};
        tx.protocol = MeshProtocol::MeshCore;
        tx.request_id = intent.request_id;
        tx.peer = intent.peer;
        tx.ok = true;
        tx.detail = 2300;
        const auto tx_effects = runtime.handleTxResult(tx, context);
        assert(tx_effects.items.size() == 1);
        const auto* delivered = effectAt<EmitActionResultEffect>(tx_effects, 0);
        assert(delivered);
        assert(delivered->action == ProtocolActionKind::TraceRoute);
        assert(delivered->state == ProtocolActionState::Delivered);
        assert(delivered->request_id == intent.request_id);

        uint8_t payload[chat::meshcore::kMeshCoreTraceBasePayloadSize] = {};
        size_t payload_len = 0;
        assert(chat::meshcore::buildTracePayload(intent.request_id,
                                                 0,
                                                 0,
                                                 payload,
                                                 sizeof(payload),
                                                 &payload_len));

        IncomingPacket packet{};
        packet.protocol = MeshProtocol::MeshCore;
        packet.payload_type = chat::meshcore::kMeshCorePayloadTypeTrace;
        packet.payload.assign(payload, payload + payload_len);
        packet.path.assign({10, 11});
        const auto rx_effects = runtime.handleIncoming(packet, context);
        assert(rx_effects.items.size() == 1);
        const auto* completed = effectAt<EmitActionResultEffect>(rx_effects, 0);
        assert(completed);
        assert(completed->action == ProtocolActionKind::TraceRoute);
        assert(completed->state == ProtocolActionState::Completed);
        assert(completed->peer == intent.peer);
        assert(completed->request_id == intent.request_id);
        assert(completed->detail == 2);
    }

    {
        TraceRouteIntent intent{};
        intent.peer = 0x44444444UL;
        intent.request_id = 0x0A0B0C0DUL;
        intent.timeout_ms = 100;
        context.now_ms = 1000;
        const auto effects = runtime.prepareOutgoing(intent, context);
        assert(effects.items.size() == 1);
        assert(effectAt<SendTraceRouteEffect>(effects, 0));

        context.now_ms = 1099;
        assert(runtime.tick(context).items.empty());

        context.now_ms = 1100;
        const auto timeout_effects = runtime.tick(context);
        assert(timeout_effects.items.size() == 1);
        const auto* timed_out = effectAt<EmitActionResultEffect>(timeout_effects, 0);
        assert(timed_out);
        assert(timed_out->action == ProtocolActionKind::TraceRoute);
        assert(timed_out->state == ProtocolActionState::TimedOut);
        assert(timed_out->peer == intent.peer);
        assert(timed_out->request_id == intent.request_id);
    }

    {
        MeshCoreAppAckRegistration ack{};
        ack.signature = 0xABCDEF01UL;
        ack.peer = 0x55555555UL;
        ack.portnum = 0x1001;
        ack.timeout_ms = 500;
        context.now_ms = 2000;
        assert(runtime.trackAppAck(ack, context).items.empty());
        assert(runtime.bindAppAckToMessage(ack.signature, 77).items.empty());

        context.now_ms = 2075;
        const auto effects = runtime.handleAppAck(ack.signature, context);
        assert(effects.items.size() == 1);
        const auto* completed = effectAt<EmitActionResultEffect>(effects, 0);
        assert(completed);
        assert(completed->protocol == MeshProtocol::MeshCore);
        assert(completed->action == ProtocolActionKind::SendText);
        assert(completed->state == ProtocolActionState::Completed);
        assert(completed->peer == ack.peer);
        assert(completed->request_id == ack.signature);
        assert(completed->message_id == 77);
        assert(completed->detail == 75);
        assert(runtime.handleAppAck(ack.signature, context).items.empty());
    }

    {
        MeshCoreAppAckRegistration ack{};
        ack.signature = 0xABCDEF02UL;
        ack.peer = 0x66666666UL;
        ack.message_id = 88;
        ack.timeout_ms = 100;
        context.now_ms = 3000;
        assert(runtime.trackAppAck(ack, context).items.empty());

        context.now_ms = 3099;
        assert(runtime.tick(context).items.empty());

        context.now_ms = 3100;
        const auto effects = runtime.tick(context);
        assert(effects.items.size() == 1);
        const auto* timed_out = effectAt<EmitActionResultEffect>(effects, 0);
        assert(timed_out);
        assert(timed_out->action == ProtocolActionKind::SendText);
        assert(timed_out->state == ProtocolActionState::TimedOut);
        assert(timed_out->peer == ack.peer);
        assert(timed_out->request_id == ack.signature);
        assert(timed_out->message_id == ack.message_id);
        assert(timed_out->detail == 100);
    }

    return 0;
}
