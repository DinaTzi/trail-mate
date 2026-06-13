#include "chat/runtime/meshtastic_runtime.h"
#include "chat/runtime/protocol_runtime.h"

#include <cassert>
#include <type_traits>
#include <vector>

namespace
{

class RecordingProtocolExecutor final : public chat::runtime::IProtocolEffectExecutor
{
  public:
    bool execute(const chat::runtime::ProtocolEffect& effect) override
    {
        effects.push_back(effect);
        return next_result;
    }

    template <typename T>
    const T* effectAt(size_t index) const
    {
        if (index >= effects.size())
        {
            return nullptr;
        }
        return std::get_if<T>(&effects[index]);
    }

    bool next_result = true;
    std::vector<chat::runtime::ProtocolEffect> effects{};
};

bool executeAll(chat::runtime::IProtocolEffectExecutor& executor,
                const chat::runtime::ProtocolEffects& effects)
{
    bool ok = true;
    for (const auto& effect : effects.items)
    {
        ok = executor.execute(effect) && ok;
    }
    return ok;
}

} // namespace

int main()
{
    static_assert(std::is_base_of<chat::runtime::IProtocolEffectExecutor,
                                  RecordingProtocolExecutor>::value,
                  "RecordingProtocolExecutor must implement the effect bridge");

    {
        chat::runtime::SendDiscoverRequestEffect effect{};
        effect.protocol = chat::MeshProtocol::MeshCore;
        effect.tag = 0xAABBCCDDUL;
        effect.type_filter = 0x7FU;
        effect.prefix_only = true;
        effect.since = 1781258000UL;
        effect.rx_guard_ms = 9000;

        chat::runtime::ProtocolEffects effects{};
        effects.add(effect);
        RecordingProtocolExecutor executor{};
        assert(executeAll(executor, effects));
        assert(executor.effects.size() == 1);

        const auto* discover =
            executor.effectAt<chat::runtime::SendDiscoverRequestEffect>(0);
        assert(discover);
        assert(discover->protocol == chat::MeshProtocol::MeshCore);
        assert(discover->tag == effect.tag);
        assert(discover->type_filter == effect.type_filter);
        assert(discover->prefix_only == effect.prefix_only);
        assert(discover->since == effect.since);
        assert(discover->rx_guard_ms == effect.rx_guard_ms);
    }

    {
        chat::runtime::MeshtasticRuntime runtime{};
        chat::runtime::MeshtasticPkiResyncInput input{};
        input.cause = chat::runtime::MeshtasticPkiResyncCause::PeerKeyStale;
        input.peer = 0x12345678UL;
        input.channel = chat::ChannelId::SECONDARY;
        input.request_id = 0xCAFEUL;

        RecordingProtocolExecutor executor{};
        assert(executeAll(executor, runtime.handlePkiResync(input)));
        assert(executor.effects.size() == 3);

        const auto* forget =
            executor.effectAt<chat::runtime::ForgetPeerKeyEffect>(0);
        assert(forget);
        assert(forget->protocol == chat::MeshProtocol::Meshtastic);
        assert(forget->peer == input.peer);

        const auto* node_info =
            executor.effectAt<chat::runtime::SendNodeInfoEffect>(1);
        assert(node_info);
        assert(node_info->protocol == chat::MeshProtocol::Meshtastic);
        assert(node_info->channel == input.channel);
        assert(node_info->peer == input.peer);
        assert(node_info->want_response);

        const auto* routing_error =
            executor.effectAt<chat::runtime::SendRoutingErrorEffect>(2);
        assert(routing_error);
        assert(routing_error->protocol == chat::MeshProtocol::Meshtastic);
        assert(routing_error->channel == input.channel);
        assert(routing_error->peer == input.peer);
        assert(routing_error->request_id == input.request_id);
    }

    {
        RecordingProtocolExecutor executor{};
        executor.next_result = false;

        chat::runtime::SendSelfAnnouncementEffect effect{};
        effect.protocol = chat::MeshProtocol::MeshCore;

        chat::runtime::ProtocolEffects effects{};
        effects.add(effect);

        assert(!executeAll(executor, effects));
        assert(executor.effects.size() == 1);
        assert(executor.effectAt<chat::runtime::SendSelfAnnouncementEffect>(0));
    }

    return 0;
}
