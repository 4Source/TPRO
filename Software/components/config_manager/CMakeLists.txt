message(STATUS "-------------------------------------------------------")
message(STATUS "CHECKING VARIABLE NAMES FOR: Config Manager")
message(STATUS "Variable 'CONFIG_ENABLE_CONFIG_MANAGER' is: '${CONFIG_ENABLE_CONFIG_MANAGER}'")
message(STATUS "-------------------------------------------------------")

# KConfig Config_Manager aktiv sonst nichts registrieren
set(srcs "")
set(requires espressif__json_parser espressif__cjson file_manager)
set(include_dirs include)

if(CONFIG_ENABLE_CONFIG_MANAGER)
    set(srcs 
        config_manager.cpp)
    list(APPEND requires
    
        ) 
endif()

message(STATUS "Configure config-manager component: ")

# Register the component
message(STATUS "\tRegister config-manager component with:")
message(STATUS "\t\tSource: ${srcs}")
message(STATUS "\t\tInclude: ${include_dirs}")
message(STATUS "\t\tRequires: ${requires}")
idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES ${requires})

message(STATUS "Configure config-manager component done")