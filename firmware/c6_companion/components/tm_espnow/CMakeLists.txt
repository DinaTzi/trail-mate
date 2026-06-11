idf_component_register(
    SRCS "tm_espnow.c"
    INCLUDE_DIRS
        "include"
        "../../../../modules/core_hostlink/include"
    REQUIRES
        tm_services
        tm_wifi
        esp_wifi
)
