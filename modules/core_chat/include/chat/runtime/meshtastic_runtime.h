#pragma once

#include "chat/runtime/meshtastic_position_core.h"
#include "chat/runtime/meshtastic_waypoint_core.h"
#include "chat/runtime/protocol_runtime.h"
#include "meshtastic/mesh.pb.h"
#include "meshtastic/portnums.pb.h"
#include "pb_encode.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace chat::runtime
{

enum class MeshtasticPkiResyncCause : uint8_t
{
    LocalPkiNotReady = 0,
    LocalNoChannel,
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
            routing.error_code = routingErrorForCause(input.cause);
            effects.add(routing);
        }

        return effects;
    }

  private:
    static bool shouldReplyWithRoutingError(MeshtasticPkiResyncCause cause)
    {
        return cause == MeshtasticPkiResyncCause::LocalPkiNotReady ||
               cause == MeshtasticPkiResyncCause::LocalNoChannel ||
               cause == MeshtasticPkiResyncCause::PeerKeyMissing ||
               cause == MeshtasticPkiResyncCause::PeerKeyStale;
    }

    static int32_t routingErrorForCause(MeshtasticPkiResyncCause cause)
    {
        return cause == MeshtasticPkiResyncCause::LocalNoChannel
                   ? meshtastic_Routing_Error_NO_CHANNEL
                   : meshtastic_Routing_Error_PKI_UNKNOWN_PUBKEY;
    }
};

class MeshtasticRuntime final : public IProtocolRuntime
{
  public:
    ProtocolEffects prepareOutgoing(const ProtocolIntent& intent,
                                    const RuntimeContext& context) override
    {
        ProtocolEffects effects{};
        std::visit(
            [&effects, &context](const auto& item)
            {
                using Intent = std::decay_t<decltype(item)>;
                if constexpr (std::is_same_v<Intent, TraceRouteIntent>)
                {
                    resolveTraceRoute(item, context, effects);
                }
                else if constexpr (std::is_same_v<Intent, ExchangePositionIntent>)
                {
                    resolveExchangePosition(item, context, effects);
                }
                else if constexpr (std::is_same_v<Intent, SharePositionIntent>)
                {
                    resolveSharePosition(item, context, effects);
                }
                else if constexpr (std::is_same_v<Intent, ShareWaypointIntent>)
                {
                    resolveShareWaypoint(item, context, effects);
                }
            },
            intent);
        return effects;
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
    static constexpr uint32_t kTraceRouteRequestSalt = 0x4D545254UL;
    static constexpr uint32_t kPositionRequestSalt = 0x4D54504FUL;

    static NodeId normalizePeer(NodeId peer)
    {
        return peer == 0xFFFFFFFFUL ? 0 : peer;
    }

    static MessageId makeRequestId(MessageId requested,
                                   NodeId peer,
                                   const RuntimeContext& context,
                                   uint32_t salt)
    {
        if (requested != 0)
        {
            return requested;
        }
        MessageId id = context.now_ms ^ context.self_node ^ peer ^ salt;
        return id == 0 ? 1 : id;
    }

    static EmitActionResultEffect buildFailedAction(ProtocolActionKind action,
                                                    NodeId peer,
                                                    MessageId request_id,
                                                    int32_t detail)
    {
        EmitActionResultEffect failed{};
        failed.protocol = MeshProtocol::Meshtastic;
        failed.action = action;
        failed.state = ProtocolActionState::Failed;
        failed.peer = peer;
        failed.request_id = request_id;
        failed.detail = detail;
        return failed;
    }

    static void resolveTraceRoute(const TraceRouteIntent& intent,
                                  const RuntimeContext& context,
                                  ProtocolEffects& effects)
    {
        const NodeId peer = normalizePeer(intent.peer);
        const MessageId request_id = makeRequestId(intent.request_id,
                                                   peer,
                                                   context,
                                                   kTraceRouteRequestSalt);
        if (peer == 0 || peer == context.self_node)
        {
            effects.add(buildFailedAction(ProtocolActionKind::TraceRoute,
                                          peer,
                                          request_id,
                                          -1));
            return;
        }

        meshtastic_RouteDiscovery route = meshtastic_RouteDiscovery_init_zero;
        uint8_t route_buf[96] = {};
        pb_ostream_t stream = pb_ostream_from_buffer(route_buf, sizeof(route_buf));
        if (!pb_encode(&stream, meshtastic_RouteDiscovery_fields, &route))
        {
            effects.add(buildFailedAction(ProtocolActionKind::TraceRoute,
                                          peer,
                                          request_id,
                                          -2));
            return;
        }

        SendPacketEffect packet{};
        packet.protocol = MeshProtocol::Meshtastic;
        packet.channel = intent.channel;
        packet.dest = peer;
        packet.portnum = meshtastic_PortNum_TRACEROUTE_APP;
        packet.request_id = request_id;
        packet.want_ack = true;
        packet.want_response = true;
        packet.payload.assign(route_buf, route_buf + stream.bytes_written);
        effects.add(std::move(packet));
    }

    static void resolveExchangePosition(const ExchangePositionIntent& intent,
                                        const RuntimeContext& context,
                                        ProtocolEffects& effects)
    {
        const NodeId peer = normalizePeer(intent.peer);
        const MessageId request_id = makeRequestId(intent.request_id,
                                                   peer,
                                                   context,
                                                   kPositionRequestSalt);
        if (peer == 0 || peer == context.self_node)
        {
            effects.add(buildFailedAction(ProtocolActionKind::ExchangePosition,
                                          peer,
                                          request_id,
                                          -1));
            return;
        }

        SendPacketEffect packet{};
        packet.protocol = MeshProtocol::Meshtastic;
        packet.channel = intent.channel;
        packet.dest = peer;
        packet.portnum = meshtastic_PortNum_POSITION_APP;
        packet.request_id = request_id;
        packet.want_ack = false;
        packet.want_response = true;
        effects.add(std::move(packet));
    }

    static void resolveSharePosition(const SharePositionIntent& intent,
                                     const RuntimeContext& context,
                                     ProtocolEffects& effects)
    {
        (void)context;
        MeshtasticPositionInput input{};
        input.valid = intent.valid;
        input.latitude_deg = intent.latitude_deg;
        input.longitude_deg = intent.longitude_deg;
        input.has_altitude = intent.has_altitude;
        input.altitude_m = intent.altitude_m;
        input.has_speed = intent.has_speed;
        input.speed_mps = intent.speed_mps;
        input.has_course = intent.has_course;
        input.course_deg = intent.course_deg;
        input.satellites = intent.satellites;
        input.timestamp_s = intent.timestamp_s;

        uint8_t payload[meshtastic_Position_size] = {};
        size_t payload_len = sizeof(payload);
        if (!MeshtasticPositionCore::buildPositionPayload(input, payload, &payload_len))
        {
            effects.add(buildFailedAction(ProtocolActionKind::SharePosition,
                                          normalizePeer(intent.peer),
                                          0,
                                          -2));
            return;
        }

        SendPacketEffect packet{};
        packet.protocol = MeshProtocol::Meshtastic;
        packet.channel = intent.channel;
        packet.dest = normalizePeer(intent.peer);
        packet.portnum = meshtastic_PortNum_POSITION_APP;
        packet.want_ack = intent.want_ack && packet.dest != 0;
        packet.want_response = intent.want_response;
        packet.payload.assign(payload, payload + payload_len);
        effects.add(std::move(packet));
    }

    static void resolveShareWaypoint(const ShareWaypointIntent& intent,
                                     const RuntimeContext& context,
                                     ProtocolEffects& effects)
    {
        (void)context;
        MeshtasticWaypointInput input{};
        input.valid = intent.valid;
        input.latitude_deg = intent.latitude_deg;
        input.longitude_deg = intent.longitude_deg;
        input.id = intent.id;
        input.expire = intent.expire;
        input.locked_to = intent.locked_to;
        input.icon = intent.icon;
        input.name = intent.name;
        input.description = intent.description;

        uint8_t payload[meshtastic_Waypoint_size] = {};
        size_t payload_len = sizeof(payload);
        if (!MeshtasticWaypointCore::buildWaypointPayload(input, payload, &payload_len))
        {
            effects.add(buildFailedAction(ProtocolActionKind::ShareWaypoint,
                                          normalizePeer(intent.peer),
                                          0,
                                          -2));
            return;
        }

        SendPacketEffect packet{};
        packet.protocol = MeshProtocol::Meshtastic;
        packet.channel = intent.channel;
        packet.dest = normalizePeer(intent.peer);
        packet.portnum = meshtastic_PortNum_WAYPOINT_APP;
        packet.want_ack = intent.want_ack && packet.dest != 0;
        packet.want_response = intent.want_response;
        packet.payload.assign(payload, payload + payload_len);
        effects.add(std::move(packet));
    }

    MeshtasticPkiResyncState pki_resync_{};
};

} // namespace chat::runtime
