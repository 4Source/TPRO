# Set source files and include directories
set(srcs 
        config_manager.cpp)
set(include_dirs include)
set(requires )

message(STATUS "Configure config-manager component: ")

# Register the component
message(STATUS "\tRegister config-manager component with:")
message(STATUS "\t\tSource: ${srcs}")
message(STATUS "\t\tInclude: ${include_dirs}")
message(STATUS "\t\tRequires: ${requires}")
idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES  ${requires})

message(STATUS "Configure config-manager component done")