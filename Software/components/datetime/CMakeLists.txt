message(STATUS "-------------------------------------------------------")
message(STATUS "CHECKING VARIABLE NAMES FOR: Time")
message(STATUS "Variable 'CONFIG_ENABLE_DATE_TIME' is: '${CONFIG_ENABLE_DATE_TIME}'")
message(STATUS "-------------------------------------------------------")


# KConfig Time aktiv sonst nichts registrieren
set(srcs "")
set(requires "")
set(include_dirs include)

if(CONFIG_ENABLE_DATE_TIME)
    set(srcs 
        datetime.cpp)
    set(requires )
endif()

message(STATUS "Configure datetime component: ")

# Register the component
message(STATUS "\tRegister datetime component with:")
message(STATUS "\t\tSource: ${srcs}")
message(STATUS "\t\tInclude: ${include_dirs}")
message(STATUS "\t\tRequires: ${requires}")
idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES ${requires})

message(STATUS "Configure datetime component done")