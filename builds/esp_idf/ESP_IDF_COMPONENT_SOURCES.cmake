# Final ESP-IDF component source ownership for migrated surfaces.
#
# The source list owner is this build entrypoint plus the final app, platform,
# and module owners. Do not restore historical app/component roots to include
# this file.

set(TRAILMATE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")

set(TRAILMATE_ESP_IDF_APP_SHELL_SOURCES
    "${TRAILMATE_ROOT}/apps/esp32_lvgl/src/esp32_lvgl_idf_app_registry.cpp"
    "${TRAILMATE_ROOT}/apps/esp32_lvgl/src/esp32_lvgl_idf_app_runtime_access.cpp"
    "${TRAILMATE_ROOT}/apps/esp32_lvgl/src/esp32_lvgl_app_shell.cpp"
    "${TRAILMATE_ROOT}/apps/esp32_lvgl/src/esp32_lvgl_historical_source_descriptor.cpp"
    "${TRAILMATE_ROOT}/apps/esp32_lvgl/src/esp32_lvgl_startup_runtime.cpp"
    "${TRAILMATE_ROOT}/apps/esp32_lvgl/src/esp32_lvgl_loop_runtime.cpp"
    "${TRAILMATE_ROOT}/apps/esp32_lvgl/src/esp32_lvgl_runtime_config.cpp")

set(TRAILMATE_ESP_IDF_PRODUCT_COMPOSITION_SOURCES
    "${TRAILMATE_ROOT}/modules/product_composition/src/target_profile.cpp"
    "${TRAILMATE_ROOT}/modules/product_composition/src/target_build_binding.cpp")

set(TRAILMATE_ESP_IDF_CORE_HOSTLINK_SOURCES
    "${TRAILMATE_ROOT}/modules/core_hostlink/src/c6_frame_codec_c.c"
    "${TRAILMATE_ROOT}/modules/core_hostlink/src/c6_frame_codec.cpp")

set(TRAILMATE_ESP_IDF_CORE_SYS_SOURCES
    "${TRAILMATE_ROOT}/modules/core_sys/src/app/app_facade_access.cpp"
    "${TRAILMATE_ROOT}/modules/core_sys/src/sys/clock.cpp"
    "${TRAILMATE_ROOT}/modules/core_sys/src/platform/ui/timezone_profile.cpp")

set(TRAILMATE_ESP_IDF_UI_SHARED_SOURCES
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/assets/alert.c"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/assets/ble_topbar.c"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/assets/gps_topbar.c"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/assets/logo.c"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/assets/message_topbar.c"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/assets/route_topbar.c"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/assets/team_topbar.c"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/assets/tracker_topbar.c"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/assets/wifi_topbar.c"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/app_runtime.cpp"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/formatters.cpp"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/loop_shell.cpp"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/startup_shell.cpp"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/ui_boot.cpp"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/ui_status.cpp"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/watch_face.cpp"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/assets/fonts/font_utils.cpp"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/i18n/resource_pack_registry.cpp"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/menu/dashboard/dashboard_compass_widget.cpp"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/menu/dashboard/dashboard_gps_widget.cpp"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/menu/dashboard/dashboard_mesh_widget.cpp"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/menu/dashboard/dashboard_recent_widget.cpp"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/menu/dashboard/dashboard_state.cpp"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/menu/dashboard/dashboard_style.cpp"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/menu/menu_dashboard.cpp"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/menu/menu_layout.cpp"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/menu/menu_profile.cpp"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/menu/menu_runtime.cpp"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/page/page_profile.cpp"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/presentation_sources/runtime_gps_status_source.cpp"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/runtime/memory_profile.cpp"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/widgets/system_notification.cpp"
    "${TRAILMATE_ROOT}/modules/ui_shared/src/ui/assets/Setting.c")

set(TRAILMATE_ESP_IDF_UI_PRESENTATION_SOURCES
    "${TRAILMATE_ROOT}/modules/ui_presentation/src/gps/gps_status_model.cpp"
    "${TRAILMATE_ROOT}/modules/ui_presentation/src/menu/menu_model.cpp")

set(TRAILMATE_ESP_IDF_UI_LVGL_UX_PACK_SOURCES
    "${TRAILMATE_ROOT}/modules/ui_lvgl_ux_packs/src/packs/cardputer_compact_ux_pack.cpp"
    "${TRAILMATE_ROOT}/modules/ui_lvgl_ux_packs/src/packs/compatibility_ux_pack.cpp"
    "${TRAILMATE_ROOT}/modules/ui_lvgl_ux_packs/src/packs/simulator_full_ux_pack.cpp"
    "${TRAILMATE_ROOT}/modules/ui_lvgl_ux_packs/src/packs/tiny_node_status_ux_pack.cpp"
    "${TRAILMATE_ROOT}/modules/ui_lvgl_ux_packs/src/packs/uconsole_desktop_ux_pack.cpp"
    "${TRAILMATE_ROOT}/modules/ui_lvgl_ux_packs/src/ux/input_binding_set.cpp"
    "${TRAILMATE_ROOT}/modules/ui_lvgl_ux_packs/src/ux/screen_registry.cpp"
    "${TRAILMATE_ROOT}/modules/ui_lvgl_ux_packs/src/ux/ux_menu_provider.cpp"
    "${TRAILMATE_ROOT}/modules/ui_lvgl_ux_packs/src/ux/ux_pack_registry.cpp"
    "${TRAILMATE_ROOT}/modules/ui_lvgl_ux_packs/src/ux/ux_screen_menu_adapter.cpp")

set(TRAILMATE_ESP_IDF_PLATFORM_COMMON_SOURCES
    "${TRAILMATE_ROOT}/platform/esp/boards/src/board_runtime.cpp"
    "${TRAILMATE_ROOT}/platform/esp/idf_common/src/app_runtime_support.cpp"
    "${TRAILMATE_ROOT}/platform/esp/idf_common/src/bsp_runtime.cpp"
    "${TRAILMATE_ROOT}/platform/esp/idf_common/src/c6_companion_runtime.cpp"
    "${TRAILMATE_ROOT}/platform/esp/idf_common/src/debug/sd_coredump_export.cpp"
    "${TRAILMATE_ROOT}/platform/esp/idf_common/src/gps_runtime.cpp"
    "${TRAILMATE_ROOT}/platform/esp/idf_common/src/platform_ui_device_runtime.cpp"
    "${TRAILMATE_ROOT}/platform/esp/idf_common/src/platform_ui_gps_runtime.cpp"
    "${TRAILMATE_ROOT}/platform/esp/idf_common/src/platform_ui_team_ui_store_runtime.cpp"
    "${TRAILMATE_ROOT}/platform/esp/idf_common/src/platform_ui_settings_store.cpp"
    "${TRAILMATE_ROOT}/platform/esp/idf_common/src/platform_ui_time_runtime.cpp"
    "${TRAILMATE_ROOT}/platform/esp/idf_common/src/platform_ui_tracker_runtime.cpp"
    "${TRAILMATE_ROOT}/platform/esp/idf_common/src/platform_ui_wifi_runtime.cpp"
    "${TRAILMATE_ROOT}/platform/esp/idf_common/src/startup_support.cpp"
    "${TRAILMATE_ROOT}/platform/esp/idf_common/src/screen_sleep.cpp"
    "${TRAILMATE_ROOT}/platform/esp/idf_common/src/sx126x_radio.cpp"
    "${TRAILMATE_ROOT}/platform/esp/idf_common/src/ui_common.cpp"
    "${TRAILMATE_ROOT}/platform/esp/idf_common/src/ui_dispatcher.cpp"
    "${TRAILMATE_ROOT}/platform/esp/idf_common/src/platform_ui_wireless_companion_runtime.cpp"
    "${TRAILMATE_ROOT}/platform/esp/radio/meshtastic_radio_adapter.cpp")

set(TRAILMATE_ESP_IDF_TAB5_BOARD_SOURCES
    "${TRAILMATE_ROOT}/boards/tab5/src/codec_compat.cpp"
    "${TRAILMATE_ROOT}/boards/tab5/src/heading_runtime.cpp"
    "${TRAILMATE_ROOT}/boards/tab5/src/rtc_runtime.cpp"
    "${TRAILMATE_ROOT}/boards/tab5/src/tab5_board.cpp")

set(TRAILMATE_ESP_IDF_T_DISPLAY_P4_BOARD_SOURCES
    "${TRAILMATE_ROOT}/boards/t_display_p4/src/rtc_runtime.cpp"
    "${TRAILMATE_ROOT}/boards/t_display_p4/src/runtime_support.cpp"
    "${TRAILMATE_ROOT}/boards/t_display_p4/src/t_display_p4_board.cpp")

set(TRAILMATE_ESP_IDF_FINAL_INCLUDE_DIRS
    "${TRAILMATE_ROOT}/apps/esp32_lvgl/src"
    "${TRAILMATE_ROOT}/modules/core_hostlink/include"
    "${TRAILMATE_ROOT}/modules/core_sys/include"
    "${TRAILMATE_ROOT}/modules/core_chat/include"
    "${TRAILMATE_ROOT}/modules/core_chat/generated"
    "${TRAILMATE_ROOT}/modules/core_chat/third_party/nanopb"
    "${TRAILMATE_ROOT}/modules/core_device/include"
    "${TRAILMATE_ROOT}/modules/core_gps/include"
    "${TRAILMATE_ROOT}/modules/core_mesh/include"
    "${TRAILMATE_ROOT}/modules/core_phone/include"
    "${TRAILMATE_ROOT}/modules/core_team/include"
    "${TRAILMATE_ROOT}/modules/product_composition/include"
    "${TRAILMATE_ROOT}/modules/chat_presentation_adapters/include"
    "${TRAILMATE_ROOT}/modules/ui_chat_runtime/include"
    "${TRAILMATE_ROOT}/modules/ui_gps_runtime/include"
    "${TRAILMATE_ROOT}/modules/ui_key_verification_runtime/include"
    "${TRAILMATE_ROOT}/modules/ui_lvgl_core/include"
    "${TRAILMATE_ROOT}/modules/ui_lvgl_ux_packs/include"
    "${TRAILMATE_ROOT}/modules/ui_map_runtime/include"
    "${TRAILMATE_ROOT}/modules/ui_presentation/include"
    "${TRAILMATE_ROOT}/modules/ui_shared/include"
    "${TRAILMATE_ROOT}/platform/esp/arduino_common/include"
    "${TRAILMATE_ROOT}/platform/esp/boards/include"
    "${TRAILMATE_ROOT}/platform/esp/common/include"
    "${TRAILMATE_ROOT}/platform/esp/idf_common/include"
    "${TRAILMATE_ROOT}/platform/shared/include"
    "${TRAILMATE_ROOT}/boards/tab5/include"
    "${TRAILMATE_ROOT}/boards/t_display_p4/include"
    "${TRAILMATE_ROOT}")
