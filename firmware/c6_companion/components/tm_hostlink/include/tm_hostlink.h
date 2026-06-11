#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

    esp_err_t tm_hostlink_init(void);
    esp_err_t tm_hostlink_start(void);
    const char* tm_hostlink_state_name(void);

#ifdef __cplusplus
}
#endif
