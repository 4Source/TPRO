# Set source files and include directories
set(srcs 
        light_effect_manager.cpp)
set(include_dirs include)
set(requires 
        effects 
        led
        task_handles
        config_manager
        time)

message(STATUS "Configure light-effect-manager component: ")

# Register the component
message(STATUS "\tRegister light-effect-manager component with:")
message(STATUS "\t\tSource: ${srcs}")
message(STATUS "\t\tInclude: ${include_dirs}")
message(STATUS "\t\tRequires: ${requires}")
idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES  ${requires})

message(STATUS "Configure light-effect-manager component done")