# Set source files and include directories
set(srcs 
        day_night_effect.cpp)
set(include_dirs include) #effects.hpp
set(requires 
        espressif__json_parser 
        led)

message(STATUS "Configure effects component: ")

# TODO: Is it possible to only have a single effects.hpp which includes all files
# # Create auto generated header for embed files 
# message(STATUS "\tCreate combined header: ")
# set(COMBINED_HEADER "${COMPONENT_DIR}/effects.hpp")
# file(WRITE ${COMBINED_HEADER} "#pragma once \n\n // Auto-generated file\n\n")
# file(GLOB HEADER "include/*.hpp")
# foreach(FILE IN LISTS HEADER)
#     file(APPEND ${COMBINED_HEADER}
#         "#include \"${FILE}\"\n"
#     )
# endforeach()


# Register the component
message(STATUS "\tRegister effects component with:")
message(STATUS "\t\tSource: ${srcs}")
message(STATUS "\t\tInclude: ${include_dirs}")
message(STATUS "\t\tRequires: ${requires}")
idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES  ${requires})

message(STATUS "Configure effects component done")

# idf_component_register(
#     SRC_DIRS "day_night" "."
#     INCLUDE_DIRS "interface" "day_night"
#     REQUIRES led espressif__json_parser
# )