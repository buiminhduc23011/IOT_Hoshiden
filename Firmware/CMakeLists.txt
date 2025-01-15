cmake_minimum_required(VERSION 3.5)
set(EXTRA_COMPONENT_DIRS    ${CMAKE_CURRENT_SOURCE_DIR}/component/socketio-esp-idf
                            ${CMAKE_CURRENT_SOURCE_DIR}/component)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

string(TIMESTAMP BUILD_DATE "%Y%m%d_%H%M%S")

set(FW_MAJOR 1)
set(FW_MINOR 0)
set(FW_BUILD 2)
set(PROJECT_VER_NUMBER 103)
set(FW_NAME "IOT_HOSHIDEN_${BUILD_DATE}")
set(PROJECT_VER "${FW_MAJOR}.${FW_MINOR}.${FW_BUILD}_${BUILD_DATE}")
set(PROJECT_NAME "${FW_NAME}")

add_definitions(-DFW_VERSION_MAJOR=${FW_MAJOR})
add_definitions(-DFW_VERSION_MINOR=${FW_MINOR})
add_definitions(-DFW_VERSION_BUILD=${FW_BUILD})
add_definitions(-DFW_VERSION=${PROJECT_VER_NUMBER})
add_definitions(-DFW_BUILD_DATE="${BUILD_DATE}")
add_definitions(-DPROJECT_NAME="${FW_NAME}")

project(${PROJECT_NAME})

idf_build_set_property(COMPILE_OPTIONS "-Wno-format;" APPEND)
