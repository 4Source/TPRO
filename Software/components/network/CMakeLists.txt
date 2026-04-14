# Set source files and include directories
set(srcs 
        network.cpp)
set(include_dirs include)
set(requires 
        esp_driver_gpio
        esp_driver_spi
        esp_netif 
        esp_wifi 
        esp_eth)

message(STATUS "Configure network component: ")

if(CONFIG_CONNECT_ETHERNET)
    message(STATUS "\tEthernet is enabled")
    list(APPEND srcs ethernet_network.cpp)
endif()

if(CONFIG_CONNECT_WIFI)
    message(STATUS "\tWifi is enabled")
    message(STATUS "\tWifi SSID: ${CONFIG_WIFI_SSID}")
    message(STATUS "\tWifi PW: ${CONFIG_WIFI_PASSWORD}")

    if("${CONFIG_WIFI_SSID}" STREQUAL "myssid") 
        message(WARNING "Your config contains default ssid for wifi!")
    endif()

    if("${CONFIG_WIFI_PASSWORD}" STREQUAL "mypassword")
        message(WARNING "Your config contains default password for wifi!")
    endif()

    list(APPEND srcs wifi_network.cpp)
endif()

# Register the component
message(STATUS "\tRegister network component with:")
message(STATUS "\t\tSource: ${srcs}")
message(STATUS "\t\tInclude: ${include_dirs}")
message(STATUS "\t\tRequires: ${requires}")
idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES  ${requires})

message(STATUS "Configure network component done")