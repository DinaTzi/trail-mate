#include "chat/infra/mesh_adapter_router_core.h"

#include <cassert>
#include <memory>
#include <string>

namespace
{

class FakeMeshAdapter final : public chat::IMeshAdapter
{
  public:
    explicit FakeMeshAdapter(chat::NodeId node_id) : node_id_(node_id) {}

    bool sendText(chat::ChannelId channel, const std::string& text,
                  chat::MessageId* out_msg_id, chat::NodeId peer = 0) override
    {
        ++send_count;
        last_channel = channel;
        last_peer = peer;
        last_text = text;
        if (out_msg_id)
        {
            *out_msg_id = next_message_id;
        }
        return send_ok;
    }

    bool pollIncomingText(chat::MeshIncomingText*) override
    {
        return false;
    }

    bool sendAppData(chat::ChannelId, uint32_t, const uint8_t*, size_t,
                     chat::NodeId = 0, bool = false, chat::MessageId = 0,
                     bool = false) override
    {
        return false;
    }

    bool pollIncomingData(chat::MeshIncomingData*) override
    {
        return false;
    }

    chat::MeshActionResult triggerDiscoveryActionDetailed(
        chat::MeshDiscoveryAction action) override
    {
        ++discovery_count;
        last_discovery = action;
        return discovery_result;
    }

    void applyConfig(const chat::MeshConfig&) override {}

    bool isReady() const override
    {
        return ready;
    }

    bool pollIncomingRawPacket(uint8_t*, size_t& out_len, size_t) override
    {
        out_len = 0;
        return false;
    }

    chat::NodeId getNodeId() const override
    {
        return node_id_;
    }

    chat::NodeId node_id_ = 0;
    bool ready = true;
    bool send_ok = true;
    int send_count = 0;
    int discovery_count = 0;
    chat::MessageId next_message_id = 42;
    chat::ChannelId last_channel = chat::ChannelId::PRIMARY;
    chat::NodeId last_peer = 0;
    std::string last_text;
    chat::MeshDiscoveryAction last_discovery = chat::MeshDiscoveryAction::ScanLocal;
    chat::MeshActionResult discovery_result = chat::MeshActionResult::success();
};

} // namespace

int main()
{
    chat::MeshAdapterRouterCore router;

    auto meshcore_backend = std::unique_ptr<FakeMeshAdapter>(
        new FakeMeshAdapter(0x4D430001UL));
    FakeMeshAdapter* meshcore = meshcore_backend.get();
    meshcore->discovery_result =
        chat::MeshActionResult::fail(chat::MeshOperationFailure::RadioOffline);

    assert(router.installBackend(chat::MeshProtocol::MeshCore,
                                 std::move(meshcore_backend)));
    assert(router.backendProtocol() == chat::MeshProtocol::MeshCore);
    assert(router.hasBackend());
    assert(router.getNodeId() == 0x4D430001UL);

    chat::MeshActionResult discovery =
        router.triggerDiscoveryActionDetailed(chat::MeshDiscoveryAction::ScanLocal);
    assert(!discovery.ok);
    assert(discovery.failure == chat::MeshOperationFailure::RadioOffline);
    assert(meshcore->discovery_count == 1);
    assert(meshcore->last_discovery == chat::MeshDiscoveryAction::ScanLocal);

    auto meshtastic_backend = std::unique_ptr<FakeMeshAdapter>(
        new FakeMeshAdapter(0x11112222UL));
    FakeMeshAdapter* meshtastic = meshtastic_backend.get();

    assert(router.installBackend(chat::MeshProtocol::Meshtastic,
                                 std::move(meshtastic_backend)));
    assert(router.backendProtocol() == chat::MeshProtocol::Meshtastic);
    assert(router.getNodeId() == 0x11112222UL);

    chat::MeshSendResult sent =
        router.sendTextDetailed(chat::ChannelId::PRIMARY, "hello", 0, 0x44);
    assert(sent.ok);
    assert(sent.msg_id == 42);
    assert(meshtastic->send_count == 1);
    assert(meshtastic->last_text == "hello");
    assert(meshtastic->last_peer == 0x44);

    router.setActiveProtocol(chat::MeshProtocol::MeshCore);
    assert(router.backendProtocol() == chat::MeshProtocol::MeshCore);
    discovery =
        router.triggerDiscoveryActionDetailed(chat::MeshDiscoveryAction::SendIdBroadcast);
    assert(!discovery.ok);
    assert(discovery.failure == chat::MeshOperationFailure::RadioOffline);
    assert(meshcore->discovery_count == 2);
    assert(meshcore->last_discovery == chat::MeshDiscoveryAction::SendIdBroadcast);

    return 0;
}
