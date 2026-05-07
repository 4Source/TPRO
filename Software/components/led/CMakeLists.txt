message(STATUS "-------------------------------------------------------")
message(STATUS "CHECKING VARIABLE NAMES FOR: Led")
message(STATUS "Variable 'CONFIG_ENABLE_LED' is: '${CONFIG_ENABLE_LED}'")
message(STATUS "-------------------------------------------------------")

# KConfig Led aktiv sonst nichts registrieren
set(srcs "")
set(requires task_handles
        esp_driver_rmt
        esp_hal_rmt
        driver
        esp_driver_gpio)
set(include_dirs include)

if(CONFIG_ENABLE_LED)
    set(srcs "led_controller.cpp"
        "led_frame.cpp"
        "led_strip_encoder.cpp")
endif()

message(STATUS "Configure led component: ")

# Register the component
message(STATUS "\tRegister led component with:")
message(STATUS "\t\tSource: ${srcs}")
message(STATUS "\t\tInclude: ${include_dirs}")
message(STATUS "\t\tRequires: ${requires}")
idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES ${requires})

message(STATUS "Configure led component done")
