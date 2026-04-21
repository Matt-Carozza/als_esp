idf_component_register(SRCS "app_main.c"
                    PRIV_REQUIRES mqtt nvs_flash esp_netif message_router transport_mqtt protocol mobile_app
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_driver_ledc esp_driver_gpio
                    REQUIRES driver
                    PRIV_REQUIRES esp_driver_ledc)
