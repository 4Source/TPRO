message(STATUS "-------------------------------------------------------")
message(STATUS "CHECKING VARIABLE NAMES FOR: light_effect_manager")
message(STATUS "Variable 'CONFIG_ENABLE_LIGHT_EFFECT_MANAGER' is: '${CONFIG_ENABLE_LIGHT_EFFECT_MANAGER}'")
message(STATUS "-------------------------------------------------------")

# KConfig LighteffectManager aktiv sonst nichts registrieren
set(srcs "")
set(requires "led" "effects" "esp_driver_spi" "config_manager" "task_handles" "datetime")
set(include_dirs include)

if(CONFIG_ENABLE_LIGHT_EFFECT_MANAGER)
        set(srcs 
                light_effect_manager.cpp)
        list(APPEND requires 
)
endif()

message(STATUS "Configure light-effect-manager component: ")

# Register the component
message(STATUS "\tRegister light-effect-manager component with:")
message(STATUS "\t\tSource: ${srcs}")
message(STATUS "\t\tInclude: ${include_dirs}")
message(STATUS "\t\tRequires: ${requires}")
idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES ${requires})

message(STATUS "-------------------------------------------------------")
# Prüfen, ob config_manager überhaupt als Komponente existiert
idf_component_get_property(cm_dir config_manager COMPONENT_DIR)
message(STATUS "DEBUG LIGHT_EFFECT_MANAGER: config_manager path is '${cm_dir}'")

# Prüfen, welche Includes diese Komponente wirklich sieht
get_target_property(inc_dirs ${COMPONENT_LIB} INCLUDE_DIRECTORIES)
message(STATUS "DEBUG LIGHT_EFFECT_MANAGER: Current Include Dirs: ${inc_dirs}")
message(STATUS "-------------------------------------------------------")
message(STATUS "Configure light-effect-manager component done")