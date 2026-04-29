message(STATUS "-------------------------------------------------------")
message(STATUS "CHECKING VARIABLE NAMES FOR: task handles")
message(STATUS "Variable 'CONFIG_ENABLE_TASK_HANDLES' is: '${CONFIG_ENABLE_TASK_HANDLES}'")
message(STATUS "-------------------------------------------------------")


# KConfig task_handles aktiv sonst nichts registrieren
set(srcs "")
set(requires "")
set(include_dirs include)

if(CONFIG_ENABLE_TASK_HANDLES)
    # Set source files and include directories
    set(srcs )
    set(include_dirs include)
    set(requires )
endif()


message(STATUS "Configure task_handles component: ")

# Register the component
message(STATUS "\tRegister task_handles component with:")
message(STATUS "\t\tSource: ${srcs}")
message(STATUS "\t\tInclude: ${include_dirs}")
message(STATUS "\t\tRequires: ${requires}")
idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES ${requires})

message(STATUS "Configure task_handles component done")
