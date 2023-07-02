idf_component_register(SRCS "softAPSTA.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES "esp_wifi" "nvs_flash" "esp_http_server"
                    EMBED_FILES "web.html")
