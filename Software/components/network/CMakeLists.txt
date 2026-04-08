# Set source files and include directories
set(srcs 
        network.cpp)
set(include_dirs include)
set(requires 
        esp_netif 
        esp_wifi 
        esp_eth)

message(STATUS "Configure network component: ")

if(CONFIG_CONNECT_ETHERNET)
    message(STATUS "\tEthernet is enabled")
endif()

if(CONFIG_CONNECT_WIFI)
    message(STATUS "\tWifi is enabled")
    message(STATUS "\tWifi SSID: ${CONFIG_WIFI_SSID}")
    message(STATUS "\tWifi PW: ${CONFIG_WIFI_PASSWORD}")

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