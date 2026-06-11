#pragma once

#include "hostlink/c6/c6_protocol.h"

#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

    esp_err_t tm_espnow_init(void);
    esp_err_t tm_espnow_apply_config(const tm_c6_espnow_config_t* config);
    esp_err_t tm_espnow_send_packet(const tm_c6_espnow_packet_t* packet);

#ifdef __cplusplus
}
#endif
