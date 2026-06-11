#pragma once

#include "hostlink/c6/c6_protocol.h"

#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

    esp_err_t tm_wifi_init(void);
    esp_err_t tm_wifi_ensure_radio_started(void);
    esp_err_t tm_wifi_apply_config(const tm_c6_wifi_config_t* config);
    esp_err_t tm_wifi_handle_control(const tm_c6_wifi_control_t* control);

#ifdef __cplusplus
}
#endif
