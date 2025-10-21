#include "wifi/WifiScanner.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

#define DEFAULT_SCAN_LIST_SIZE 20
static const char *TAG = "WifiScanner";

static void print_ap_to_buf(char *buf, size_t len, wifi_ap_record_t *ap_info, uint16_t count) {
    size_t offset = 0;
    offset += snprintf(buf + offset, len - offset, "[");
    for (int i = 0; i < count && offset < len; i++) {
        offset += snprintf(buf + offset, len - offset,
                           "{\"ssid\":\"%s\",\"rssi\":%d,\"chan\":%d}%s",
                           ap_info[i].ssid,
                           ap_info[i].rssi,
                           ap_info[i].primary,
                           (i + 1 < count) ? "," : "");
    }
    snprintf(buf + offset, len - offset, "]");
}

esp_err_t device_scan_networks(char *buf, size_t len) {
    if (!buf || len == 0)
        return ESP_ERR_INVALID_ARG;

    ESP_LOGI(TAG, "Starting WiFi scan");

    // Проверяем, что Wi-Fi уже инициализирован
    wifi_mode_t mode;
    esp_err_t err = esp_wifi_get_mode(&mode);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi not initialized (err=0x%x)", err);
        return err;
    }

    // Если устройство не в STA-режиме, просто временно переключаем режим
    if (mode != WIFI_MODE_APSTA) {
        ESP_LOGI(TAG, "Temporarily switching to APSTA mode for scan");
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    }

    wifi_scan_config_t scan_config = {
        .ssid = 0,
        .bssid = 0,
        .channel = 0,
        .show_hidden = true
    };

    ESP_LOGI(TAG, "Scan start");
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
    ESP_LOGI(TAG, "Scan complete");

    uint16_t ap_count = 0;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));

    if (ap_count > DEFAULT_SCAN_LIST_SIZE)
        ap_count = DEFAULT_SCAN_LIST_SIZE;

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_info));
    ESP_LOGI(TAG, "Found %u APs", ap_count);

    print_ap_to_buf(buf, len, ap_info, ap_count);

    ESP_LOGI(TAG, "Scan results prepared");
    return ESP_OK;
}
