#include "hostlink/c6/c6_frame_codec.h"
#include "hostlink/c6/c6_frame_codec_c.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

namespace
{

void patch_le16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
{
    data[offset] = static_cast<uint8_t>(value & 0xFF);
    data[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

} // namespace

int main()
{
    using namespace hostlink::c6;

    static_assert(sizeof(tm_c6_frame_header_t) == TM_C6_FRAME_HEADER_LEN,
                  "C6 header must remain 20 bytes");
    static_assert(sizeof(tm_c6_ble_packet_header_t) == TM_C6_BLE_PACKET_HEADER_LEN,
                  "BLE packet header must remain 4 bytes");
    static_assert(sizeof(tm_c6_espnow_packet_t) <= TM_C6_MAX_PAYLOAD,
                  "ESP-NOW packet payload must fit in one C6 frame");
    static_assert(sizeof(tm_c6_companion_config_t) <= TM_C6_MAX_PAYLOAD,
                  "Companion config must fit in one C6 frame");
    static_assert(sizeof(tm_c6_config_report_t) <= TM_C6_MAX_PAYLOAD,
                  "Companion report must fit in one C6 frame");

    const uint8_t hello_payload[] = {0x10, 0x20, 0x30};
    EncodeRequest hello{};
    hello.frame_type = TM_C6_FRAME_HELLO;
    hello.channel = TM_C6_CH_CONTROL;
    hello.flags = TM_C6_FLAG_ACK_REQUIRED;
    hello.seq = 42;
    hello.payload = hello_payload;
    hello.payload_len = sizeof(hello_payload);

    std::vector<uint8_t> encoded;
    assert(encode_frame(hello, encoded));
    assert(encoded.size() == TM_C6_FRAME_HEADER_LEN + sizeof(hello_payload));

    DecodeResult decoded = decode_frame(encoded.data(), encoded.size());
    assert(decoded.status == DecodeStatus::Ok);
    assert(decoded.bytes_consumed == encoded.size());
    assert(decoded.frame.frame_type == TM_C6_FRAME_HELLO);
    assert(decoded.frame.channel == TM_C6_CH_CONTROL);
    assert(decoded.frame.flags == TM_C6_FLAG_ACK_REQUIRED);
    assert(decoded.frame.seq == 42);
    assert(decoded.frame.ack == 0);
    assert(decoded.frame.payload == std::vector<uint8_t>(hello_payload, hello_payload + 3));

    tm_c6_decode_result_t c_decoded =
        tm_c6_decode_frame(encoded.data(), encoded.size(), TM_C6_MAX_PAYLOAD);
    assert(c_decoded.status == TM_C6_DECODE_OK);
    assert(c_decoded.bytes_consumed == encoded.size());
    assert(c_decoded.frame.frame_type == TM_C6_FRAME_HELLO);
    assert(c_decoded.frame.channel == TM_C6_CH_CONTROL);
    assert(c_decoded.frame.flags == TM_C6_FLAG_ACK_REQUIRED);
    assert(c_decoded.frame.seq == 42);
    assert(c_decoded.frame.payload_len == sizeof(hello_payload));
    assert(std::memcmp(c_decoded.frame.payload, hello_payload, sizeof(hello_payload)) == 0);

    std::vector<uint8_t> bad_magic = encoded;
    bad_magic[0] = 0;
    decoded = decode_frame(bad_magic.data(), bad_magic.size());
    assert(decoded.status == DecodeStatus::BadMagic);
    assert(decode_status_error_code(decoded.status) == TM_C6_ERROR_BAD_MAGIC);

    std::vector<uint8_t> bad_version = encoded;
    bad_version[4] = 2;
    decoded = decode_frame(bad_version.data(), bad_version.size());
    assert(decoded.status == DecodeStatus::BadVersion);
    assert(std::strcmp(decode_status_name(decoded.status), "bad_version") == 0);

    std::vector<uint8_t> bad_crc = encoded;
    bad_crc.back() ^= 0x40;
    decoded = decode_frame(bad_crc.data(), bad_crc.size());
    assert(decoded.status == DecodeStatus::BadCrc);
    assert(decode_status_error_code(decoded.status) == TM_C6_ERROR_BAD_CRC);

    std::vector<uint8_t> too_large = encoded;
    patch_le16(too_large, 14, static_cast<uint16_t>(TM_C6_MAX_PAYLOAD + 1));
    decoded = decode_frame(too_large.data(), too_large.size());
    assert(decoded.status == DecodeStatus::PayloadTooLarge);

    std::vector<uint8_t> oversized_payload(TM_C6_MAX_PAYLOAD + 1, 0xAA);
    EncodeRequest oversized{};
    oversized.payload = oversized_payload.data();
    oversized.payload_len = oversized_payload.size();
    assert(!encode_frame(oversized, encoded));

    EncodeRequest ack{};
    ack.frame_type = TM_C6_FRAME_ACK;
    ack.channel = TM_C6_CH_CONTROL;
    ack.flags = TM_C6_FLAG_IS_ACK;
    ack.seq = 43;
    ack.ack = 42;
    assert(encode_frame(ack, encoded));
    decoded = decode_frame(encoded.data(), encoded.size());
    assert(decoded.status == DecodeStatus::Ok);
    assert(decoded.frame.flags == TM_C6_FLAG_IS_ACK);
    assert(decoded.frame.ack == 42);

    tm_c6_ping_t c_ping{};
    c_ping.nonce = 0xC60000AAu;
    c_ping.uptime_ms = 12345;
    tm_c6_encode_request_t c_request{};
    c_request.frame_type = TM_C6_FRAME_PING;
    c_request.channel = TM_C6_CH_CONTROL;
    c_request.flags = TM_C6_FLAG_ACK_REQUIRED;
    c_request.seq = 55;
    c_request.payload = reinterpret_cast<const uint8_t*>(&c_ping);
    c_request.payload_len = sizeof(c_ping);
    uint8_t c_encoded[TM_C6_FRAME_HEADER_LEN + sizeof(tm_c6_ping_t)] = {};
    size_t c_encoded_len = 0;
    assert(tm_c6_encode_frame(&c_request,
                              c_encoded,
                              sizeof(c_encoded),
                              &c_encoded_len,
                              TM_C6_MAX_PAYLOAD));
    decoded = decode_frame(c_encoded, c_encoded_len);
    assert(decoded.status == DecodeStatus::Ok);
    assert(decoded.frame.frame_type == TM_C6_FRAME_PING);
    assert(decoded.frame.channel == TM_C6_CH_CONTROL);
    assert(decoded.frame.flags == TM_C6_FLAG_ACK_REQUIRED);
    assert(decoded.frame.seq == 55);
    assert(decoded.frame.payload.size() == sizeof(c_ping));

    const uint8_t meshcore_payload[] = {0x7E};
    EncodeRequest meshcore{};
    meshcore.frame_type = TM_C6_FRAME_BLE_UPLINK;
    meshcore.channel = TM_C6_CH_BLE_MESHCORE;
    meshcore.seq = 44;
    meshcore.payload = meshcore_payload;
    meshcore.payload_len = sizeof(meshcore_payload);
    assert(encode_frame(meshcore, encoded));
    decoded = decode_frame(encoded.data(), encoded.size());
    assert(decoded.status == DecodeStatus::Ok);
    assert(decoded.frame.frame_type == TM_C6_FRAME_BLE_UPLINK);
    assert(decoded.frame.channel == TM_C6_CH_BLE_MESHCORE);

    EncodeRequest fragment{};
    fragment.frame_type = TM_C6_FRAME_BLE_DOWNLINK;
    fragment.channel = TM_C6_CH_BLE_TRAILMATE;
    fragment.flags = TM_C6_FLAG_IS_FRAGMENT | TM_C6_FLAG_FRAGMENT_START |
                     TM_C6_FLAG_FRAGMENT_END;
    assert(encode_frame(fragment, encoded));
    decoded = decode_frame(encoded.data(), encoded.size());
    assert(decoded.status == DecodeStatus::Ok);
    assert((decoded.frame.flags & TM_C6_FLAG_IS_FRAGMENT) != 0);
    assert((decoded.frame.flags & TM_C6_FLAG_FRAGMENT_START) != 0);
    assert((decoded.frame.flags & TM_C6_FLAG_FRAGMENT_END) != 0);

    decoded = decode_frame(encoded.data(), encoded.size() - 1);
    assert(decoded.status == DecodeStatus::Incomplete);

    tm_c6_companion_config_t config{};
    config.config_seq = 7;
    config.requested_features = TM_C6_FEATURE_BLE_TRAILMATE | TM_C6_FEATURE_ESPNOW_TEAM;
    config.ble.ble_enabled = 1;
    config.ble.trailmate_enabled = 1;
    config.ble.pairing_mode = TM_C6_PAIRING_FIXED_PIN;
    config.ble.preferred_mtu = 244;
    std::memcpy(config.ble.device_name, "TrailMate", 9);
    config.espnow.espnow_enabled = 1;
    config.espnow.broadcast_mac[0] = 0xFF;
    config.espnow.broadcast_mac[1] = 0xFF;
    config.espnow.broadcast_mac[2] = 0xFF;
    config.espnow.broadcast_mac[3] = 0xFF;
    config.espnow.broadcast_mac[4] = 0xFF;
    config.espnow.broadcast_mac[5] = 0xFF;

    EncodeRequest config_set{};
    config_set.frame_type = TM_C6_FRAME_CONFIG_SET;
    config_set.channel = TM_C6_CH_CONTROL;
    config_set.flags = TM_C6_FLAG_ACK_REQUIRED;
    config_set.seq = 77;
    config_set.payload = reinterpret_cast<const uint8_t*>(&config);
    config_set.payload_len = sizeof(config);
    assert(encode_frame(config_set, encoded));
    decoded = decode_frame(encoded.data(), encoded.size());
    assert(decoded.status == DecodeStatus::Ok);
    assert(decoded.frame.frame_type == TM_C6_FRAME_CONFIG_SET);
    assert(decoded.frame.channel == TM_C6_CH_CONTROL);
    assert(decoded.frame.payload.size() == sizeof(config));
    const auto* decoded_config =
        reinterpret_cast<const tm_c6_companion_config_t*>(decoded.frame.payload.data());
    assert(decoded_config->config_seq == 7);
    assert(decoded_config->ble.preferred_mtu == 244);
    assert(decoded_config->espnow.broadcast_mac[5] == 0xFF);

    return 0;
}
