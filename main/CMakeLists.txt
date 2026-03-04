idf_component_register(SRCS "app_main.c"
                    PRIV_REQUIRES mqtt nvs_flash esp_netif message_router transport_mqtt protocol driver esp_adc daylight daylight_buffer
                 INCLUDE_DIRS "include")
