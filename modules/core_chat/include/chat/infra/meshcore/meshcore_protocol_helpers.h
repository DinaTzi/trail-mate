/**
 * @file meshcore_protocol_helpers.h
 * @brief Shared MeshCore protocol helper utilities
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace chat
{
namespace meshcore
{

struct ParsedPacket
{
    uint8_t route_type = 0;
    uint8_t payload_type = 0;
    uint8_t payload_ver = 0;
    size_t path_len_index = 0;
    size_t path_len = 0;
    const uint8_t* path = nullptr;
    const uint8_t* payload = nullptr;
    size_t payload_len = 0;
};

constexpr size_t kMeshCoreTraceBasePayloadSize = 9;
constexpr uint8_t kMeshCorePayloadTypeTrace = 0x09;

struct DecodedTracePayload
{
    bool valid = false;
    bool terminal = false;
    uint32_t tag = 0;
    uint32_t auth = 0;
    uint8_t flags = 0;
    size_t path_hash_size = 1;
    size_t offset = 0;
    const uint8_t* trace_hashes = nullptr;
    size_t trace_hashes_len = 0;
    uint8_t next_hash = 0;
};

std::string toHex(const uint8_t* data, size_t len, size_t max_len = 128);
uint8_t buildHeader(uint8_t route_type, uint8_t payload_type, uint8_t payload_ver);
bool parsePacket(const uint8_t* data, size_t len, ParsedPacket* out);
size_t tracePathHashSize(uint8_t flags);
bool isValidTracePathHashBytes(uint8_t flags, size_t path_hashes_len, size_t max_hops);
bool buildTracePayload(uint32_t tag, uint32_t auth, uint8_t flags,
                       const uint8_t* path_hashes, size_t path_hashes_len,
                       uint8_t* out_payload, size_t out_cap, size_t* out_len);
bool buildTracePayload(uint32_t tag, uint32_t auth, uint8_t flags,
                       uint8_t* out_payload, size_t out_cap, size_t* out_len);
bool decodeTracePayload(const uint8_t* payload, size_t payload_len,
                        size_t path_len, DecodedTracePayload* out);
uint32_t hashFrame(const uint8_t* data, size_t len);
uint32_t packetSignature(uint8_t payload_type, size_t path_len,
                         const uint8_t* payload, size_t payload_len);
float estimateLoRaAirtimeMs(size_t frame_len, float bw_khz, uint8_t sf, uint8_t cr_denom);
uint32_t estimateSendTimeoutMs(size_t frame_len, size_t path_len, bool flood,
                               float bw_khz, uint8_t sf, uint8_t cr_denom);
uint32_t saturatingAddU32(uint32_t base, uint32_t delta);
float scoreFromSnr(float snr, uint8_t sf, size_t packet_len);
uint32_t computeRxDelayMs(float rx_delay_base, float score, uint32_t air_ms);
bool isZeroKey(const uint8_t* key, size_t len);
void toHmacKey32(const uint8_t* key16, uint8_t* out_key32);
void sharedSecretToKeys(const uint8_t* secret, uint8_t* out_key16, uint8_t* out_key32);
void sha256Trunc(uint8_t* out_hash, size_t out_len, const uint8_t* msg, size_t msg_len);
uint8_t computeChannelHash(const uint8_t* key16);
size_t aesEncrypt(const uint8_t* key16, uint8_t* dest, const uint8_t* src, size_t src_len);
size_t aesDecrypt(const uint8_t* key16, uint8_t* dest, const uint8_t* src, size_t src_len);
size_t encryptThenMac(const uint8_t* key16, const uint8_t* key32,
                      uint8_t* out, size_t out_cap,
                      const uint8_t* plain, size_t plain_len);
bool macThenDecrypt(const uint8_t* key16, const uint8_t* key32,
                    const uint8_t* src, size_t src_len, uint8_t* out_plain, size_t* out_plain_len);
size_t trimTrailingZeros(const uint8_t* buf, size_t len);

} // namespace meshcore
} // namespace chat
