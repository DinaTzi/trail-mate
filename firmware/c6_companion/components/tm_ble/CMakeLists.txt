idf_component_register(
    SRCS "tm_ble.c"
    INCLUDE_DIRS
        "include"
        "../../../../modules/core_hostlink/include"
    REQUIRES
        bt
        nvs_flash
        tm_services
)
