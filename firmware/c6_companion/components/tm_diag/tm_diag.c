#include "tm_diag.h"

#include "esp_heap_caps.h"
#include "esp_log.h"

static const char* TAG = "C6_DIAG";

esp_err_t tm_diag_init(void)
{
    ESP_LOGI(TAG, "diag initialized free_heap=%lu", (unsigned long)heap_caps_get_free_size(MALLOC_CAP_8BIT));
    return ESP_OK;
}
