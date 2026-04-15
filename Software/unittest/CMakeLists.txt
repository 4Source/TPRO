cmake_minimum_required(VERSION 3.22)
project(light-wall-system-tests)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_C_STANDARD 23)

enable_testing()

# GTest global einbinden
add_subdirectory(${CMAKE_SOURCE_DIR}/../external/googletest "${CMAKE_BINARY_DIR}/googletest_build")

# Unterordner hinzufügen
add_subdirectory(light_effect_manager)
add_subdirectory(websocket)
add_subdirectory(effect_factory)