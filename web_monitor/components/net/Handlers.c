#include "Handlers.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>


static const char *TAG = "Handler";


// --- reboot
esp_err_t device_reboot(void)
{
    ESP_LOGW(TAG, "Reboot requested, restarting...");
    esp_restart();
    return ESP_OK; // never reached
}

// --- sysinfo
esp_err_t device_get_sysinfo(char *buf, size_t len)
{
    if (!buf || len == 0) return ESP_ERR_INVALID_ARG;

    uint64_t uptime_us = esp_timer_get_time();
    uint32_t uptime_s = uptime_us / 1000000ULL;
    uint32_t h = uptime_s / 3600;
    uint32_t m = (uptime_s % 3600) / 60;
    uint32_t s = uptime_s % 60;

    uint32_t free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);

    snprintf(buf, len,
             "{\"firmware\":\"v1.0.0\",\"uptime\":\"%02" PRIu32 ":%02" PRIu32 ":%02" PRIu32 "\","
             "\"freeHeap\":\"%" PRIu32 " KB\",\"flash\":\"%" PRIu32 " KB\"}",
             h, m, s, free_heap / 1024, flash_size / 1024);

    return ESP_OK;
}

// --- wifi scan
esp_err_t device_scan_networks(char *buf, size_t len)
{
    if (!buf || len == 0) return ESP_ERR_INVALID_ARG;
    // Пример заглушки
    strncpy(buf,
            "[{\"ssid\":\"HomeNetwork\",\"rssi\":-42,\"chan\":6},"
            "{\"ssid\":\"CafeFreeWiFi\",\"rssi\":-78,\"chan\":11},"
            "{\"ssid\":\"ESP32-Setup\",\"rssi\":-33,\"chan\":1}]",
            len);
    return ESP_OK;
}

// --- clients
esp_err_t device_get_clients(char *buf, size_t len)
{
    if (!buf || len == 0) return ESP_ERR_INVALID_ARG;
    // Пример заглушки
    strncpy(buf,
            "[{\"mac\":\"AA:BB:CC:11:22:33\",\"ip\":\"192.168.4.2\",\"last\":\"5s\"},"
            "{\"mac\":\"DE:AD:BE:EF:00:01\",\"ip\":\"192.168.4.3\",\"last\":\"23s\"}]",
            len);
    return ESP_OK;
}

// --- logs
esp_err_t device_get_logs(char *buf, size_t len)
{
    if (!buf || len == 0) return ESP_ERR_INVALID_ARG;
    strncpy(buf,
            "2025-10-19 12:00:00 System start\n"
            "2025-10-19 12:00:03 WiFi AP started\n"
            "2025-10-19 12:01:02 Scan completed\n",
            len);
    return ESP_OK;
}
