idf_component_register(
    SRCS "wifi/AccessPoint.c"
         "wifi/WifiScanner.c"
         "WebServer.c"
         "Handlers.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_wifi esp_event esp_netif esp_http_server esp_timer spi_flash lwip log
)
