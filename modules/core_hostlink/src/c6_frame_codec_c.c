#include "hostlink/c6/c6_frame_codec_c.h"

#include <string.h>

enum
{
    TM_C6_MAGIC_OFFSET = 0,
    TM_C6_VERSION_OFFSET = 4,
    TM_C6_HEADER_LEN_OFFSET = 5,
    TM_C6_FRAME_TYPE_OFFSET = 6,
    TM_C6_CHANNEL_OFFSET = 7,
    TM_C6_FLAGS_OFFSET = 8,
    TM_C6_SEQ_OFFSET = 10,
    TM_C6_ACK_OFFSET = 12,
    TM_C6_PAYLOAD_LEN_OFFSET = 14,
    TM_C6_CRC_OFFSET = 16,
};

static uint16_t tm_c6_read_le16(const uint8_t* data)
{
    return (uint16_t)data[0] | (uint16_t)((uint16_t)data[1] << 8);
}

static uint32_t tm_c6_read_le32(const uint8_t* data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8) | ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

static void tm_c6_write_le16(uint8_t* out, size_t offset, uint16_t value)
{
    out[offset] = (uint8_t)(value & 0xFFu);
    out[offset + 1] = (uint8_t)((value >> 8) & 0xFFu);
}

static void tm_c6_write_le32(uint8_t* out, size_t offset, uint32_t value)
{
    out[offset] = (uint8_t)(value & 0xFFu);
    out[offset + 1] = (uint8_t)((value >> 8) & 0xFFu);
    out[offset + 2] = (uint8_t)((value >> 16) & 0xFFu);
    out[offset + 3] = (uint8_t)((value >> 24) & 0xFFu);
}

uint32_t tm_c6_crc32_ieee(const uint8_t* data, size_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    if (data == NULL && len != 0)
    {
        return 0;
    }

    for (size_t i = 0; i < len; ++i)
    {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit)
        {
            const uint32_t mask = 0u - (crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

bool tm_c6_encode_frame(const tm_c6_encode_request_t* request,
                        uint8_t* out,
                        size_t out_capacity,
                        size_t* out_len,
                        size_t max_payload)
{
    if (out_len != NULL)
    {
        *out_len = 0;
    }
    if (request == NULL || out == NULL || out_len == NULL ||
        request->payload_len > max_payload ||
        request->payload_len > TM_C6_MAX_PAYLOAD ||
        (request->payload == NULL && request->payload_len != 0))
    {
        return false;
    }

    const size_t total_len = (size_t)TM_C6_FRAME_HEADER_LEN + request->payload_len;
    if (out_capacity < total_len)
    {
        return false;
    }

    tm_c6_write_le32(out, TM_C6_MAGIC_OFFSET, TM_C6_MAGIC);
    out[TM_C6_VERSION_OFFSET] = (uint8_t)TM_C6_PROTO_VERSION;
    out[TM_C6_HEADER_LEN_OFFSET] = (uint8_t)TM_C6_FRAME_HEADER_LEN;
    out[TM_C6_FRAME_TYPE_OFFSET] = request->frame_type;
    out[TM_C6_CHANNEL_OFFSET] = request->channel;
    tm_c6_write_le16(out, TM_C6_FLAGS_OFFSET, request->flags);
    tm_c6_write_le16(out, TM_C6_SEQ_OFFSET, request->seq);
    tm_c6_write_le16(out, TM_C6_ACK_OFFSET, request->ack);
    tm_c6_write_le16(out, TM_C6_PAYLOAD_LEN_OFFSET, (uint16_t)request->payload_len);
    tm_c6_write_le32(out, TM_C6_CRC_OFFSET, 0);

    if (request->payload_len > 0)
    {
        memcpy(out + TM_C6_FRAME_HEADER_LEN, request->payload, request->payload_len);
    }

    const uint32_t crc = tm_c6_crc32_ieee(out, total_len);
    tm_c6_write_le32(out, TM_C6_CRC_OFFSET, crc);
    *out_len = total_len;
    return true;
}

tm_c6_decode_result_t tm_c6_decode_frame(const uint8_t* data, size_t len, size_t max_payload)
{
    tm_c6_decode_result_t result;
    memset(&result, 0, sizeof(result));
    result.status = TM_C6_DECODE_INCOMPLETE;

    if (data == NULL || len < TM_C6_FRAME_HEADER_LEN)
    {
        return result;
    }

    if (tm_c6_read_le32(data + TM_C6_MAGIC_OFFSET) != TM_C6_MAGIC)
    {
        result.status = TM_C6_DECODE_BAD_MAGIC;
        return result;
    }

    if (data[TM_C6_VERSION_OFFSET] != TM_C6_PROTO_VERSION)
    {
        result.status = TM_C6_DECODE_BAD_VERSION;
        return result;
    }

    if (data[TM_C6_HEADER_LEN_OFFSET] != TM_C6_FRAME_HEADER_LEN)
    {
        result.status = TM_C6_DECODE_BAD_HEADER_LENGTH;
        return result;
    }

    const uint16_t payload_len = tm_c6_read_le16(data + TM_C6_PAYLOAD_LEN_OFFSET);
    if (payload_len > max_payload || payload_len > TM_C6_MAX_PAYLOAD)
    {
        result.status = TM_C6_DECODE_PAYLOAD_TOO_LARGE;
        return result;
    }

    const size_t total_len = (size_t)TM_C6_FRAME_HEADER_LEN + (size_t)payload_len;
    if (len < total_len)
    {
        result.status = TM_C6_DECODE_INCOMPLETE;
        return result;
    }

    uint8_t crc_header[TM_C6_FRAME_HEADER_LEN];
    memcpy(crc_header, data, TM_C6_FRAME_HEADER_LEN);
    memset(crc_header + TM_C6_CRC_OFFSET, 0, 4);

    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < TM_C6_FRAME_HEADER_LEN; ++i)
    {
        crc ^= crc_header[i];
        for (int bit = 0; bit < 8; ++bit)
        {
            const uint32_t mask = 0u - (crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    for (size_t i = 0; i < (size_t)payload_len; ++i)
    {
        crc ^= data[TM_C6_FRAME_HEADER_LEN + i];
        for (int bit = 0; bit < 8; ++bit)
        {
            const uint32_t mask = 0u - (crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    crc = ~crc;

    if (tm_c6_read_le32(data + TM_C6_CRC_OFFSET) != crc)
    {
        result.status = TM_C6_DECODE_BAD_CRC;
        return result;
    }

    result.status = TM_C6_DECODE_OK;
    result.bytes_consumed = total_len;
    result.frame.frame_type = data[TM_C6_FRAME_TYPE_OFFSET];
    result.frame.channel = data[TM_C6_CHANNEL_OFFSET];
    result.frame.flags = tm_c6_read_le16(data + TM_C6_FLAGS_OFFSET);
    result.frame.seq = tm_c6_read_le16(data + TM_C6_SEQ_OFFSET);
    result.frame.ack = tm_c6_read_le16(data + TM_C6_ACK_OFFSET);
    result.frame.payload = data + TM_C6_FRAME_HEADER_LEN;
    result.frame.payload_len = payload_len;
    return result;
}

const char* tm_c6_decode_status_name(tm_c6_decode_status_t status)
{
    switch (status)
    {
    case TM_C6_DECODE_OK:
        return "ok";
    case TM_C6_DECODE_INCOMPLETE:
        return "incomplete";
    case TM_C6_DECODE_BAD_MAGIC:
        return "bad_magic";
    case TM_C6_DECODE_BAD_VERSION:
        return "bad_version";
    case TM_C6_DECODE_BAD_HEADER_LENGTH:
        return "bad_header_length";
    case TM_C6_DECODE_PAYLOAD_TOO_LARGE:
        return "payload_too_large";
    case TM_C6_DECODE_BAD_CRC:
        return "bad_crc";
    }
    return "unknown";
}

tm_c6_error_code_t tm_c6_decode_status_error_code(tm_c6_decode_status_t status)
{
    switch (status)
    {
    case TM_C6_DECODE_OK:
        return TM_C6_OK;
    case TM_C6_DECODE_BAD_MAGIC:
        return TM_C6_ERROR_BAD_MAGIC;
    case TM_C6_DECODE_BAD_VERSION:
        return TM_C6_ERROR_BAD_VERSION;
    case TM_C6_DECODE_BAD_CRC:
        return TM_C6_ERROR_BAD_CRC;
    case TM_C6_DECODE_PAYLOAD_TOO_LARGE:
        return TM_C6_ERROR_PAYLOAD_TOO_LARGE;
    case TM_C6_DECODE_INCOMPLETE:
    case TM_C6_DECODE_BAD_HEADER_LENGTH:
        return TM_C6_ERROR_INTERNAL;
    }
    return TM_C6_ERROR_INTERNAL;
}
