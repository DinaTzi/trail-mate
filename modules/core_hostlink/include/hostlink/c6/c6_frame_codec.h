#pragma once

#include "hostlink/c6/c6_protocol.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace hostlink::c6
{

enum class DecodeStatus
{
    Ok,
    Incomplete,
    BadMagic,
    BadVersion,
    BadHeaderLength,
    PayloadTooLarge,
    BadCrc,
};

struct Frame
{
    uint8_t frame_type = TM_C6_FRAME_ERROR;
    uint8_t channel = TM_C6_CH_CONTROL;
    uint16_t flags = 0;
    uint16_t seq = 0;
    uint16_t ack = 0;
    std::vector<uint8_t> payload;
};

struct EncodeRequest
{
    uint8_t frame_type = TM_C6_FRAME_PING;
    uint8_t channel = TM_C6_CH_CONTROL;
    uint16_t flags = 0;
    uint16_t seq = 0;
    uint16_t ack = 0;
    const uint8_t* payload = nullptr;
    size_t payload_len = 0;
};

struct DecodeResult
{
    DecodeStatus status = DecodeStatus::Incomplete;
    Frame frame{};
    size_t bytes_consumed = 0;
};

uint32_t crc32_ieee(const uint8_t* data, size_t len);
bool encode_frame(const EncodeRequest& request,
                  std::vector<uint8_t>& out,
                  size_t max_payload = TM_C6_MAX_PAYLOAD);
DecodeResult decode_frame(const uint8_t* data,
                          size_t len,
                          size_t max_payload = TM_C6_MAX_PAYLOAD);
const char* decode_status_name(DecodeStatus status);
tm_c6_error_code_t decode_status_error_code(DecodeStatus status);

} // namespace hostlink::c6
