idf_component_register(SRCS "main.c" 
                              "config_manager.c"
                              "auth.c"
                              "web_server.c"
                              "wifi_manager.c"
                              "ethernet_manager.c"
                              "bluetooth_manager.c"
                              "mqtt_manager.c"
                       INCLUDE_DIRS "."
                       EMBED_FILES "../web/login.html"
                                   "../web/index.html"
                                   "../config.ini"
                       REQUIRES esp_http_server
                                esp_wifi
                                esp_netif
                                esp_eth
                                nvs_flash
                                mbedtls
                                spiffs
                                json
                                esp_timer
                                vfs
                                bt
                                mqtt
                                ini_parser)
