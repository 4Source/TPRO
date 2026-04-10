set(requires 
        network 
        webserver 
        config_manager
        nvs_flash
        led 
        effects
        esp_driver_gpio
        log
        driver)

idf_component_register(SRCS "main.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES ${requires})
