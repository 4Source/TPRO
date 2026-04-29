message(STATUS "-------------------------------------------------------")
message(STATUS "CHECKING VARIABLE NAMES FOR: APIS")
message(STATUS "Variable 'CONFIG_ENABLE_APIS' is: '${CONFIG_ENABLE_APIS}'")
message(STATUS "-------------------------------------------------------")

# KConfig APIS aktiv sonst nichts registrieren
set(srcs "")
set(requires "datetime" "esp_https_server")
set(include_dirs include)

if(CONFIG_ENABLE_APIS)
    set(srcs timeserver.cpp)
    list(APPEND requires
    
        ) 

endif()

message(STATUS "Configure apis component: ")

# Register the component
message(STATUS "\tRegister apis component with:")
message(STATUS "\t\tSource: ${srcs}")
message(STATUS "\t\tInclude: ${include_dirs}")
message(STATUS "\t\tRequires: ${requires}")
idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES ${requires})

message(STATUS "Configure apis component done")