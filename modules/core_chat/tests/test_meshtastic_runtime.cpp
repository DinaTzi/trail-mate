#include "chat/runtime/meshtastic_runtime.h"

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
    using chat::runtime::ForgetPeerKeyEffect;
    using chat::runtime::MeshtasticPkiResyncCause;
    using chat::runtime::MeshtasticPkiResyncInput;
    using chat::runtime::MeshtasticRuntime;
    using chat::runtime::SendNodeInfoEffect;
    using chat::runtime::SendRoutingErrorEffect;

    MeshtasticRuntime runtime;

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
        input.cause = MeshtasticPkiResyncCause::PeerKeyMissing;
        input.peer = 0;
        input.request_id = 0x4004UL;

        const auto effects = runtime.handlePkiResync(input);
        assert(effects.empty());
    }

    return 0;
}
