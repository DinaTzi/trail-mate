#pragma once

#include "hostlink/c6/c6_protocol.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum tm_c6_decode_status
    {
        TM_C6_DECODE_OK = 0,
        TM_C6_DECODE_INCOMPLETE = 1,
        TM_C6_DECODE_BAD_MAGIC = 2,
        TM_C6_DECODE_BAD_VERSION = 3,
        TM_C6_DECODE_BAD_HEADER_LENGTH = 4,
        TM_C6_DECODE_PAYLOAD_TOO_LARGE = 5,
        TM_C6_DECODE_BAD_CRC = 6,
    } tm_c6_decode_status_t;

    typedef struct tm_c6_frame_view
    {
        uint8_t frame_type;
        uint8_t channel;
        uint16_t flags;
        uint16_t seq;
        uint16_t ack;
        const uint8_t* payload;
        size_t payload_len;
    } tm_c6_frame_view_t;

    typedef struct tm_c6_encode_request
    {
        uint8_t frame_type;
        uint8_t channel;
        uint16_t flags;
        uint16_t seq;
        uint16_t ack;
        const uint8_t* payload;
        size_t payload_len;
    } tm_c6_encode_request_t;

    typedef struct tm_c6_decode_result
    {
        tm_c6_decode_status_t status;
        tm_c6_frame_view_t frame;
        size_t bytes_consumed;
    } tm_c6_decode_result_t;

    uint32_t tm_c6_crc32_ieee(const uint8_t* data, size_t len);
    bool tm_c6_encode_frame(const tm_c6_encode_request_t* request,
                            uint8_t* out,
                            size_t out_capacity,
                            size_t* out_len,
                            size_t max_payload);
    tm_c6_decode_result_t tm_c6_decode_frame(const uint8_t* data, size_t len, size_t max_payload);
    const char* tm_c6_decode_status_name(tm_c6_decode_status_t status);
    tm_c6_error_code_t tm_c6_decode_status_error_code(tm_c6_decode_status_t status);

#ifdef __cplusplus
}
#endif
