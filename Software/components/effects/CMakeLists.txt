message(STATUS "-------------------------------------------------------")
message(STATUS "CHECKING VARIABLE NAMES FOR: effects")
message(STATUS "Variable 'CONFIG_ENABLE_EFFECTS' is: '${CONFIG_ENABLE_EFFECTS}'")
message(STATUS "-------------------------------------------------------")

# KConfig Effects aktiv sonst nichts registrieren
set(srcs "")
set(requires "datetime" "config_manager" "espressif__cjson" "file_manager" "espressif__json_parser" "led" "lvgl")
set(include_dirs "include")

# Quelldateien nur hinzufügen, wenn das Modul aktiv ist
if(CONFIG_ENABLE_EFFECTS)
    list(APPEND srcs 
        "column_scan_effect.cpp"
        "debug_effect.cpp"
        "row_scan_effect.cpp"
        "static_color_effect.cpp"
        "day_night_effect.cpp"
        "effect_factory.cpp"
        "blinking_effect.cpp"
        "breathing_effect.cpp"
        "color_wipe_effect.cpp"
        "color_hsv.cpp"
        "continent_effect.cpp"
        "day_night_effect.cpp"
        "dvd_effect.cpp"
        "equalizer_effect.cpp"
        "fire_effect.cpp"
        "matrix_effect.cpp"
        "plasma_effect.cpp"
        "rainbow_wave_effect.cpp"
        "timeline_effect.cpp"
        "scrolling_effect.cpp")
    list(APPEND requires 
)
endif()
# Set source files and include directories

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
                       REQUIRES ${requires})

message(STATUS "Configure effects component done")
