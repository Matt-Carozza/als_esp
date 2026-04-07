idf_component_register(SRCS "occupancy.c"
                    REQUIRES protocol transport_mqtt esp_driver_uart
                    INCLUDE_DIRS "include")
