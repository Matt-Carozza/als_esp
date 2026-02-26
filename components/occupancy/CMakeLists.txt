idf_component_register(SRCS "occupancy.c"
                    REQUIRES protocol transport_mqtt
                    INCLUDE_DIRS "include")
