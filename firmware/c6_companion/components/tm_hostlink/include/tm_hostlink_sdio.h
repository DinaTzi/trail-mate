#pragma once

#include "esp_err.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    esp_err_t tm_hostlink_sdio_init(void);
    esp_err_t tm_hostlink_sdio_recv(uint8_t* data, size_t max_len, size_t* out_len, uint32_t timeout_ms);
    esp_err_t tm_hostlink_sdio_send(const uint8_t* data, size_t len, uint32_t timeout_ms);
    void tm_hostlink_sdio_poll_tx_done(void);

#ifdef __cplusplus
}
#endif
