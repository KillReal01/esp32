idf_component_register(
    SRCS "wifi/AccessPoint.cpp"
         "wifi/WifiScanner.cpp"
         "WebServer.cpp"
         "Handlers.cpp"
         "LogCapture.cpp"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_wifi esp_event esp_netif esp_http_server esp_timer spi_flash lwip log cjson
)
