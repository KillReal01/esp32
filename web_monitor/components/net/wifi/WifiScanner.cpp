#include "wifi/WifiScanner.h"

#include <cstdarg>
#include <cstring>
#include <cstdio>

#include "esp_wifi.h"
#include "esp_log.h"

#define DEFAULT_SCAN_LIST_SIZE 5

static const char *TAG = "WifiScanner";

static void append_to_buf(char *buf, size_t len, size_t *offset, const char *format, ...)
{
    if (!buf || !offset || *offset >= len) {
        return;
    }

    va_list args;
    va_start(args, format);
    int written = std::vsnprintf(buf + *offset, len - *offset, format, args);
    va_end(args);

    if (written < 0) {
        return;
    }

    size_t written_sz = static_cast<size_t>(written);
    if (written_sz >= len - *offset) {
        *offset = len - 1;
        return;
    }

    *offset += written_sz;
}

static void print_ap_to_buf(char *buf, size_t len, wifi_ap_record_t *ap_info, uint16_t count)
{
    size_t offset = 0;
    append_to_buf(buf, len, &offset, "[");
    for (int i = 0; i < count && offset < len; i++) {
        size_t ssid_len = strnlen(reinterpret_cast<const char *>(ap_info[i].ssid), sizeof(ap_info[i].ssid));
        append_to_buf(buf, len, &offset,
                      "{\"ssid\":\"%.*s\",\"rssi\":%d,\"chan\":%d}%s",
                      static_cast<int>(ssid_len),
                      reinterpret_cast<const char *>(ap_info[i].ssid),
                      ap_info[i].rssi,
                      ap_info[i].primary,
                      (i + 1 < count) ? "," : "");
    }
    append_to_buf(buf, len, &offset, "]");
}

esp_err_t device_scan_networks(char *buf, size_t len)
{
    if (!buf || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Starting WiFi scan");

    wifi_mode_t mode;
    esp_err_t err = esp_wifi_get_mode(&mode);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi not initialized (err=0x%x)", err);
        return err;
    }

    if (mode != WIFI_MODE_APSTA) {
        ESP_LOGI(TAG, "Temporarily switching to APSTA mode for scan");
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    }

    wifi_scan_config_t scan_config{};
    scan_config.show_hidden = true;

    ESP_LOGI(TAG, "Scan start");
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
    ESP_LOGI(TAG, "Scan complete");

    uint16_t ap_count = 0;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));

    if (ap_count > DEFAULT_SCAN_LIST_SIZE) {
        ap_count = DEFAULT_SCAN_LIST_SIZE;
    }

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_info));
    ESP_LOGI(TAG, "Found %u APs", ap_count);

    print_ap_to_buf(buf, len, ap_info, ap_count);

    ESP_LOGI(TAG, "Scan results prepared");
    return ESP_OK;
}
