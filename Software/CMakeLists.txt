# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.22)

# Include the ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Add custom components path to the project. By default all components in this path will be build into the project
list(APPEND EXTRA_COMPONENT_DIRS components)

# Set the project name
project(light-wall)
