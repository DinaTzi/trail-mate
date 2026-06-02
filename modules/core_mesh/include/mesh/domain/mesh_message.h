#pragma once

#include "mesh/domain/mesh_packet.h"
#include "mesh/domain/node_id.h"
#include "mesh/domain/peer_identity.h"

#include <stdint.h>

namespace mesh
{

enum class MeshProtocolEventKind : uint8_t
{
    None = 0,
    MessageReceived,
    PeerDiscovered,
    PeerKeyLearned,
    PeerAdvertReceived,
    AckReceived,
    Duplicate,
};

struct MeshAdvertProfile
{
    bool has_name = false;
    char name[32]{};
    bool has_location = false;
    int32_t latitude_i6 = 0;
    int32_t longitude_i6 = 0;
    uint8_t node_type = 0;
    uint32_t timestamp = 0;
    uint8_t hops = 0xFF;
};

struct MeshProtocolEvent
{
    MeshProtocolEventKind kind = MeshProtocolEventKind::None;
    NodeId peer{};
    PeerPublicKey peer_key{};
    MeshAdvertProfile advert{};
    ByteView payload{};
    uint32_t packet_id = 0;
    uint8_t payload_bytes[256]{};
    size_t payload_size = 0;

    void setPayload(const uint8_t* data, size_t size)
    {
        payload = ByteView{};
        payload_size = 0;
        if (!data || size == 0 || size > sizeof(payload_bytes))
        {
            return;
        }
        for (size_t index = 0; index < size; ++index)
        {
            payload_bytes[index] = data[index];
        }
        payload_size = size;
        payload = ByteView{payload_bytes, payload_size};
    }
};

} // namespace mesh
