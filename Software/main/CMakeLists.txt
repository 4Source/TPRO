set(requires 
        network 
        https_server 
        config_manager
        nvs_flash
        led 
        light_effect_manager
        effects
        esp_driver_gpio
        log
        file_manager
        driver
        apis)

idf_component_register(SRCS "main.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES ${requires})
