set(requires 
        network 
        webserver 
        config_manager
        nvs_flash
        led 
        light_effect_manager
        effects
        esp_driver_gpio
        log
        driver
        apis)

idf_component_register(SRCS "main.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES ${requires})
