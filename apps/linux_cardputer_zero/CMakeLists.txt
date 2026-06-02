cmake_minimum_required(VERSION 3.20)

project(TrailMateLinuxCardputerZero LANGUAGES CXX)

include(CTest)

set(TRAIL_MATE_REPO_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/../.."
    CACHE PATH "Trail Mate repository root")

include("${TRAIL_MATE_REPO_ROOT}/cmake/TrailMateUxPacks.cmake")
trailmate_add_ui_lvgl_ux_packs(trailmate_ui_lvgl_ux_packs)

add_library(trailmate_linux_cardputer_zero_shell
    src/cardputer_zero_input_method_port.cpp
    src/cardputer_zero_notification_port.cpp
    src/linux_cardputer_zero_app_shell.cpp
    "${TRAIL_MATE_REPO_ROOT}/modules/product_composition/src/target_profile.cpp"
    "${TRAIL_MATE_REPO_ROOT}/modules/product_composition/src/target_ux_binding.cpp")

target_include_directories(trailmate_linux_cardputer_zero_shell
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/src"
        "${TRAIL_MATE_REPO_ROOT}"
        "${TRAIL_MATE_REPO_ROOT}/modules/product_composition/include"
        "${TRAIL_MATE_REPO_ROOT}/modules/ui_lvgl_ux_packs/include")

target_link_libraries(trailmate_linux_cardputer_zero_shell
    PUBLIC
        trailmate_ui_lvgl_ux_packs)

target_compile_features(trailmate_linux_cardputer_zero_shell PUBLIC cxx_std_17)

if(BUILD_TESTING)
    add_executable(trailmate_linux_cardputer_zero_app_shell_smoke
        tests/linux_cardputer_zero_app_shell_smoke.cpp)
    target_link_libraries(trailmate_linux_cardputer_zero_app_shell_smoke
        PRIVATE trailmate_linux_cardputer_zero_shell)
    add_test(NAME trailmate_linux_cardputer_zero_app_shell_smoke
             COMMAND trailmate_linux_cardputer_zero_app_shell_smoke)

    add_executable(trailmate_linux_cardputer_zero_page_manifest_smoke
        "${TRAIL_MATE_REPO_ROOT}/modules/ui_presentation/tests/test_page_manifest.cpp")
    target_link_libraries(trailmate_linux_cardputer_zero_page_manifest_smoke
        PRIVATE trailmate_linux_cardputer_zero_shell)
    add_test(NAME trailmate_linux_cardputer_zero_page_manifest_smoke
             COMMAND trailmate_linux_cardputer_zero_page_manifest_smoke)

    add_executable(trailmate_linux_cardputer_zero_menu_profile_smoke
        "${TRAIL_MATE_REPO_ROOT}/modules/ui_shared/tests/test_cardputer_zero_menu_profile.cpp"
        "${TRAIL_MATE_REPO_ROOT}/modules/ui_shared/src/ui/menu/menu_profile.cpp")
    target_include_directories(trailmate_linux_cardputer_zero_menu_profile_smoke
        PRIVATE
            "${TRAIL_MATE_REPO_ROOT}/modules/ui_shared/tests/stubs"
            "${TRAIL_MATE_REPO_ROOT}/modules/ui_shared/include")
    target_compile_definitions(trailmate_linux_cardputer_zero_menu_profile_smoke
        PRIVATE TRAIL_MATE_CARDPUTER_ZERO_LINUX=1)
    target_compile_features(trailmate_linux_cardputer_zero_menu_profile_smoke
        PRIVATE cxx_std_17)
    add_test(NAME trailmate_linux_cardputer_zero_menu_profile_smoke
             COMMAND trailmate_linux_cardputer_zero_menu_profile_smoke)
endif()
