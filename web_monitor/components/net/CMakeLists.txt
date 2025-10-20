idf_component_register(
    SRCS "AccessPoint.c" "WebServer.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_wifi esp_event esp_netif esp_http_server lwip log
)
