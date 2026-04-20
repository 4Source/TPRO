# Set source files and include directories
set(srcs 
        file_manager.cpp)
set(include_dirs include)
set(requires fatfs sdmmc esp_timer esp_driver_sdspi esp_driver_spi config_manager)

message(STATUS "Configure file-manager component: ")

# Register the component
message(STATUS "\tRegister file-manager component with:")
message(STATUS "\t\tSource: ${srcs}")
message(STATUS "\t\tInclude: ${include_dirs}")
message(STATUS "\t\tRequires: ${requires}")
idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES  ${requires})

message(STATUS "Configure file-manager component done")
