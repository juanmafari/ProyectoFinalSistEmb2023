idf_component_register(SRCS "MQTTThings.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES "esp_event" "esp_netif" "mqtt")
