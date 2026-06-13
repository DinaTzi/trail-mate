#pragma once

#include "chat/domain/chat_types.h"

#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace chat::runtime
{

enum class ProtocolActionKind : uint8_t
{
    Unknown = 0,
    SendText,
    RequestNodeInfo,
    TraceRoute,
    ExchangePosition,
    PkiResync,
    Discover,
};

enum class ProtocolActionState : uint8_t
{
    Pending = 0,
    Delivered,
    Completed,
    Failed,
    TimedOut,
};

struct RuntimeContext
{
    MeshProtocol protocol = MeshProtocol::Meshtastic;
    NodeId self_node = 0;
    uint32_t now_ms = 0;
};

struct SendTextIntent
{
    ChannelId channel = ChannelId::PRIMARY;
    NodeId peer = 0;
    std::string text;
};

struct RequestNodeInfoIntent
{
    NodeId peer = 0;
    bool want_response = true;
};

struct TraceRouteIntent
{
    NodeId peer = 0;
    MessageId request_id = 0;
    uint32_t auth = 0;
    uint8_t flags = 0;
    uint32_t timeout_ms = 15000;
};

struct ExchangePositionIntent
{
    NodeId peer = 0;
};

struct DiscoverIntent
{
    MeshDiscoveryAction action = MeshDiscoveryAction::ScanLocal;
    uint32_t tag = 0;
    uint8_t type_filter = 0xFF;
    bool prefix_only = false;
    uint32_t since = 0;
    uint32_t rx_guard_ms = 5000;
};

struct StartKeyVerificationIntent
{
    NodeId peer = 0;
};

using ProtocolIntent = std::variant<SendTextIntent,
                                    RequestNodeInfoIntent,
                                    TraceRouteIntent,
                                    ExchangePositionIntent,
                                    DiscoverIntent,
                                    StartKeyVerificationIntent>;

struct IncomingPacket
{
    MeshProtocol protocol = MeshProtocol::Meshtastic;
    ChannelId channel = ChannelId::PRIMARY;
    NodeId from = 0;
    NodeId to = 0;
    MessageId packet_id = 0;
    MessageId request_id = 0;
    uint32_t portnum = 0;
    uint8_t payload_type = 0;
    bool want_response = false;
    bool encrypted = false;
    std::vector<uint8_t> payload;
    std::vector<uint8_t> path;
    RxMeta rx_meta{};
};

struct TxResult
{
    MeshProtocol protocol = MeshProtocol::Meshtastic;
    MessageId request_id = 0;
    NodeId peer = 0;
    bool ok = false;
    int32_t detail = 0;
};

struct SendPacketEffect
{
    MeshProtocol protocol = MeshProtocol::Meshtastic;
    ChannelId channel = ChannelId::PRIMARY;
    NodeId dest = 0;
    uint32_t portnum = 0;
    MessageId request_id = 0;
    bool want_ack = false;
    bool want_response = false;
    std::vector<uint8_t> payload;
};

struct SendNodeInfoEffect
{
    MeshProtocol protocol = MeshProtocol::Meshtastic;
    ChannelId channel = ChannelId::PRIMARY;
    NodeId peer = 0;
    bool want_response = false;
};

struct SendRoutingErrorEffect
{
    MeshProtocol protocol = MeshProtocol::Meshtastic;
    ChannelId channel = ChannelId::PRIMARY;
    NodeId peer = 0;
    MessageId request_id = 0;
    int32_t error_code = 0;
};

struct SendTraceRouteEffect
{
    MeshProtocol protocol = MeshProtocol::MeshCore;
    NodeId peer = 0;
    MessageId request_id = 0;
    uint32_t auth = 0;
    uint8_t flags = 0;
    uint32_t timeout_ms = 0;
};

struct SendDiscoverRequestEffect
{
    MeshProtocol protocol = MeshProtocol::MeshCore;
    uint32_t tag = 0;
    uint8_t type_filter = 0xFF;
    bool prefix_only = false;
    uint32_t since = 0;
    uint32_t rx_guard_ms = 5000;
};

struct SendSelfAnnouncementEffect
{
    MeshProtocol protocol = MeshProtocol::MeshCore;
    bool broadcast = true;
    bool include_location = false;
    int32_t lat_i6 = 0;
    int32_t lon_i6 = 0;
};

struct ForgetPeerKeyEffect
{
    MeshProtocol protocol = MeshProtocol::Meshtastic;
    NodeId peer = 0;
};

struct RequestPeerNodeInfoEffect
{
    MeshProtocol protocol = MeshProtocol::Meshtastic;
    ChannelId channel = ChannelId::PRIMARY;
    NodeId peer = 0;
    bool want_response = true;
};

struct PublishIncomingTextEffect
{
    MeshIncomingText text{};
};

struct PublishIncomingDataEffect
{
    MeshIncomingData data{};
};

struct PublishNodeInfoEffect
{
    MeshProtocol protocol = MeshProtocol::MeshCore;
    ChannelId channel = ChannelId::PRIMARY;
    NodeId node_id = 0;
    std::string short_name;
    std::string long_name;
    uint32_t timestamp = 0;
    uint8_t role = 0;
    uint8_t hops = 0;
    RxMeta rx_meta{};
};

struct EmitActionResultEffect
{
    MeshProtocol protocol = MeshProtocol::Meshtastic;
    ProtocolActionKind action = ProtocolActionKind::Unknown;
    ProtocolActionState state = ProtocolActionState::Pending;
    NodeId peer = 0;
    MessageId request_id = 0;
    MessageId message_id = 0;
    int32_t detail = 0;
};

struct UpdatePeerRouteEffect
{
    MeshProtocol protocol = MeshProtocol::MeshCore;
    NodeId peer = 0;
    uint8_t next_hop = 0;
    ChannelId preferred_channel = ChannelId::PRIMARY;
};

using ProtocolEffect = std::variant<SendPacketEffect,
                                    SendNodeInfoEffect,
                                    SendRoutingErrorEffect,
                                    SendTraceRouteEffect,
                                    SendDiscoverRequestEffect,
                                    SendSelfAnnouncementEffect,
                                    ForgetPeerKeyEffect,
                                    RequestPeerNodeInfoEffect,
                                    PublishIncomingTextEffect,
                                    PublishIncomingDataEffect,
                                    PublishNodeInfoEffect,
                                    EmitActionResultEffect,
                                    UpdatePeerRouteEffect>;

struct ProtocolEffects
{
    std::vector<ProtocolEffect> items;

    bool empty() const
    {
        return items.empty();
    }

    template <typename T>
    void add(T effect)
    {
        items.emplace_back(std::move(effect));
    }
};

template <typename Visitor>
decltype(auto) visitProtocolEffect(ProtocolEffect& effect, Visitor&& visitor)
{
    return std::visit(std::forward<Visitor>(visitor), effect);
}

template <typename Visitor>
decltype(auto) visitProtocolEffect(const ProtocolEffect& effect, Visitor&& visitor)
{
    return std::visit(std::forward<Visitor>(visitor), effect);
}


class IProtocolRuntime
{
  public:
    virtual ~IProtocolRuntime() = default;

    virtual ProtocolEffects prepareOutgoing(const ProtocolIntent& intent,
                                            const RuntimeContext& context) = 0;
    virtual ProtocolEffects handleIncoming(const IncomingPacket& packet,
                                           const RuntimeContext& context) = 0;
    virtual ProtocolEffects handleTxResult(const TxResult& result,
                                           const RuntimeContext& context) = 0;
    virtual ProtocolEffects tick(const RuntimeContext& context) = 0;
};

class IProtocolEffectExecutor
{
  public:
    virtual ~IProtocolEffectExecutor() = default;
    virtual bool execute(const ProtocolEffect& effect) = 0;
};

} // namespace chat::runtime
