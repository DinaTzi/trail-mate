cmake_minimum_required(VERSION 3.20)

set(SDKCONFIG "${CMAKE_BINARY_DIR}/sdkconfig.c6_companion")
set(SDKCONFIG_DEFAULTS "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults")
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(trail-mate-c6-companion)
