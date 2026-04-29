message(STATUS "-------------------------------------------------------")
message(STATUS "CHECKING VARIABLE NAMES FOR: Filemanager")
message(STATUS "Variable 'CONFIG_ENABLE_FILE_MANAGER' is: '${CONFIG_ENABLE_FILE_MANAGER}'")
message(STATUS "-------------------------------------------------------")

# KConfig Filemanager aktiv sonst nichts registrieren
set(srcs "")
set(requires "fatfs sdmmc" "esp_timer" "esp_driver_sdspi" "esp_driver_spi" "config_manager")
set(include_dirs include)

if(CONFIG_ENABLE_FILE_MANAGER)
    set(srcs 
        file_manager.cpp)
    list(APPEND requires
        )
endif()

message(STATUS "Configure file-manager component: ")

# Register the component
message(STATUS "\tRegister file-manager component with:")
message(STATUS "\t\tSource: ${srcs}")
message(STATUS "\t\tInclude: ${include_dirs}")
message(STATUS "\t\tRequires: ${requires}")
idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES ${requires})

message(STATUS "Configure file-manager component done")
