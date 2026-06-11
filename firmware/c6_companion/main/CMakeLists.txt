idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES
        nvs_flash
        esp_event
        tm_hostlink
        tm_diag
        tm_services
        tm_wifi
        tm_espnow
        tm_ble
)
