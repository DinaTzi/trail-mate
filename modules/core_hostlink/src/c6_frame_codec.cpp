#include "hostlink/c6/c6_frame_codec.h"

#include <algorithm>
#include <cstring>

namespace hostlink::c6
{
namespace
{

constexpr size_t kMagicOffset = 0;
constexpr size_t kVersionOffset = 4;
constexpr size_t kHeaderLenOffset = 5;
constexpr size_t kFrameTypeOffset = 6;
constexpr size_t kChannelOffset = 7;
constexpr size_t kFlagsOffset = 8;
constexpr size_t kSeqOffset = 10;
constexpr size_t kAckOffset = 12;
constexpr size_t kPayloadLenOffset = 14;
constexpr size_t kCrcOffset = 16;

uint16_t read_le16(const uint8_t* data)
{
    return static_cast<uint16_t>(data[0]) |
           static_cast<uint16_t>(static_cast<uint16_t>(data[1]) << 8);
}

uint32_t read_le32(const uint8_t* data)
{
    return static_cast<uint32_t>(data[0]) |
           (static_cast<uint32_t>(data[1]) << 8) |
           (static_cast<uint32_t>(data[2]) << 16) |
           (static_cast<uint32_t>(data[3]) << 24);
}

void write_le16(std::vector<uint8_t>& out, uint16_t value)
{
    out.push_back(static_cast<uint8_t>(value & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

void write_le32(std::vector<uint8_t>& out, uint32_t value)
{
    out.push_back(static_cast<uint8_t>(value & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

void patch_le32(std::vector<uint8_t>& out, size_t offset, uint32_t value)
{
    out[offset] = static_cast<uint8_t>(value & 0xFF);
    out[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    out[offset + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    out[offset + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

} // namespace

uint32_t crc32_ieee(const uint8_t* data, size_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    if (data == nullptr && len != 0)
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

bool encode_frame(const EncodeRequest& request, std::vector<uint8_t>& out, size_t max_payload)
{
    if (request.payload_len > max_payload ||
        request.payload_len > TM_C6_MAX_PAYLOAD ||
        (request.payload == nullptr && request.payload_len != 0))
    {
        return false;
    }

    out.clear();
    out.reserve(TM_C6_FRAME_HEADER_LEN + request.payload_len);

    write_le32(out, TM_C6_MAGIC);
    out.push_back(static_cast<uint8_t>(TM_C6_PROTO_VERSION));
    out.push_back(static_cast<uint8_t>(TM_C6_FRAME_HEADER_LEN));
    out.push_back(request.frame_type);
    out.push_back(request.channel);
    write_le16(out, request.flags);
    write_le16(out, request.seq);
    write_le16(out, request.ack);
    write_le16(out, static_cast<uint16_t>(request.payload_len));
    write_le32(out, 0);

    if (request.payload_len > 0)
    {
        out.insert(out.end(), request.payload, request.payload + request.payload_len);
    }

    const uint32_t crc = crc32_ieee(out.data(), out.size());
    patch_le32(out, kCrcOffset, crc);
    return true;
}

DecodeResult decode_frame(const uint8_t* data, size_t len, size_t max_payload)
{
    DecodeResult result{};
    if (data == nullptr || len < TM_C6_FRAME_HEADER_LEN)
    {
        result.status = DecodeStatus::Incomplete;
        return result;
    }

    if (read_le32(data + kMagicOffset) != TM_C6_MAGIC)
    {
        result.status = DecodeStatus::BadMagic;
        return result;
    }

    if (data[kVersionOffset] != TM_C6_PROTO_VERSION)
    {
        result.status = DecodeStatus::BadVersion;
        return result;
    }

    if (data[kHeaderLenOffset] != TM_C6_FRAME_HEADER_LEN)
    {
        result.status = DecodeStatus::BadHeaderLength;
        return result;
    }

    const uint16_t payload_len = read_le16(data + kPayloadLenOffset);
    if (payload_len > max_payload || payload_len > TM_C6_MAX_PAYLOAD)
    {
        result.status = DecodeStatus::PayloadTooLarge;
        return result;
    }

    const size_t total_len = TM_C6_FRAME_HEADER_LEN + static_cast<size_t>(payload_len);
    if (len < total_len)
    {
        result.status = DecodeStatus::Incomplete;
        return result;
    }

    std::vector<uint8_t> crc_input(data, data + total_len);
    std::fill(crc_input.begin() + static_cast<std::ptrdiff_t>(kCrcOffset),
              crc_input.begin() + static_cast<std::ptrdiff_t>(kCrcOffset + 4),
              0);
    const uint32_t expected_crc = read_le32(data + kCrcOffset);
    const uint32_t actual_crc = crc32_ieee(crc_input.data(), crc_input.size());
    if (expected_crc != actual_crc)
    {
        result.status = DecodeStatus::BadCrc;
        return result;
    }

    result.status = DecodeStatus::Ok;
    result.bytes_consumed = total_len;
    result.frame.frame_type = data[kFrameTypeOffset];
    result.frame.channel = data[kChannelOffset];
    result.frame.flags = read_le16(data + kFlagsOffset);
    result.frame.seq = read_le16(data + kSeqOffset);
    result.frame.ack = read_le16(data + kAckOffset);
    result.frame.payload.assign(data + TM_C6_FRAME_HEADER_LEN, data + total_len);
    return result;
}

const char* decode_status_name(DecodeStatus status)
{
    switch (status)
    {
    case DecodeStatus::Ok:
        return "ok";
    case DecodeStatus::Incomplete:
        return "incomplete";
    case DecodeStatus::BadMagic:
        return "bad_magic";
    case DecodeStatus::BadVersion:
        return "bad_version";
    case DecodeStatus::BadHeaderLength:
        return "bad_header_length";
    case DecodeStatus::PayloadTooLarge:
        return "payload_too_large";
    case DecodeStatus::BadCrc:
        return "bad_crc";
    }
    return "unknown";
}

tm_c6_error_code_t decode_status_error_code(DecodeStatus status)
{
    switch (status)
    {
    case DecodeStatus::Ok:
        return TM_C6_OK;
    case DecodeStatus::BadMagic:
        return TM_C6_ERROR_BAD_MAGIC;
    case DecodeStatus::BadVersion:
        return TM_C6_ERROR_BAD_VERSION;
    case DecodeStatus::BadCrc:
        return TM_C6_ERROR_BAD_CRC;
    case DecodeStatus::PayloadTooLarge:
        return TM_C6_ERROR_PAYLOAD_TOO_LARGE;
    case DecodeStatus::Incomplete:
    case DecodeStatus::BadHeaderLength:
        return TM_C6_ERROR_INTERNAL;
    }
    return TM_C6_ERROR_INTERNAL;
}

} // namespace hostlink::c6
