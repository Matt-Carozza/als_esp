idf_component_register(SRCS "app_main.c"
                    PRIV_REQUIRES mqtt nvs_flash esp_netif message_router transport_mqtt protocol camera esp_driver_i2c
                    INCLUDE_DIRS "include")
