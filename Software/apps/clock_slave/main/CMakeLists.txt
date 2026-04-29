set(requires 
        esp_driver_gpio
        log
        freertos
        driver)

idf_component_register(SRCS "main.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES ${requires})
