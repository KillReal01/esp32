#include "wifi/WifiScanner.h"

#include <cstdarg>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>

#include "esp_wifi.h"
#include "esp_log.h"


static const char *TAG = "WifiScanner";

static std::string print_ap_to_buf(const std::vector<wifi_ap_record_t>& aps)
{
    std::string json;
    json.reserve(512);

    json += "[";

    for (size_t i = 0; i < aps.size(); ++i) {
        const auto& ap = aps[i];

        size_t ssid_len = strnlen(
            reinterpret_cast<const char*>(ap.ssid),
            sizeof(ap.ssid));

        json += "{\"ssid\":\"";
        json.append(reinterpret_cast<const char*>(ap.ssid), ssid_len);
        json += "\",\"rssi\":";
        json += std::to_string(ap.rssi);
        json += ",\"chan\":";
        json += std::to_string(ap.primary);
        json += "}";

        if (i + 1 < aps.size()) {
            json += ",";
        }
    }

    json += "]";

    return json;
}

esp_err_t device_scan_networks(std::string& out)
{
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
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));

    std::vector<wifi_ap_record_t> ap_info(ap_count);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_info.data()));

    ESP_LOGI(TAG, "Found %u APs", ap_info.size());
    out = print_ap_to_buf(ap_info);
    ESP_LOGI(TAG, "Scan results prepared");

    if (mode != WIFI_MODE_APSTA) {
        ESP_ERROR_CHECK(esp_wifi_set_mode(mode));
        ESP_LOGI(TAG, "Wifi mode restored");
    }

    return ESP_OK;
}
