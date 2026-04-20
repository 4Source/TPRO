cmake_minimum_required(VERSION 3.22)
project(light-wall-system-tests)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_C_STANDARD 23)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Explizite Flags für den Debug-Modus setzen
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
enable_testing()

# GTest global einbinden
add_subdirectory(${CMAKE_SOURCE_DIR}/../external/googletest "${CMAKE_BINARY_DIR}/googletest_build")

# Unterordner hinzufügen
add_subdirectory(datetime)
add_subdirectory(light_effect_manager)
add_subdirectory(websocket)
add_subdirectory(effect_factory)
add_subdirectory(effect_json)