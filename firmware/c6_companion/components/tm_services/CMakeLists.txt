idf_component_register(
    SRCS "tm_services.c"
    INCLUDE_DIRS
        "include"
        "../../../../modules/core_hostlink/include"
    REQUIRES
        tm_diag
    PRIV_REQUIRES
        heap
        esp_timer
)
