#include "wifi/WifiScanner.h"

#include <cstdio>
#include <cstring>
#include <utility>

#include "cJSON.h"
#include "esp_log.h"
#include "esp_wifi.h"

namespace {
constexpr const char *TAG = "WifiScanner";

std::string formatBssid(const uint8_t bssid[6])
{
    char out[18] = {0};
    std::snprintf(out, sizeof(out), "%02X:%02X:%02X:%02X:%02X:%02X",
                  bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
    return out;
}

std::string authToString(wifi_auth_mode_t mode)
{
    switch (mode) {
    case WIFI_AUTH_OPEN: return "OPEN";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-ENT";
    case WIFI_AUTH_WPA3_PSK: return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
    default: return "UNKNOWN";
    }
}

std::string cjsonToString(cJSON *root)
{
    if (!root) return "{}";
    char *rendered = cJSON_PrintUnformatted(root);
    if (!rendered) return "{}";
    std::string out(rendered);
    cJSON_free(rendered);
    return out;
}
} // namespace

std::vector<ScannedNetwork> WifiScanner::scanNetworks() const
{
    wifi_scan_config_t config{};
    config.show_hidden = true;
    ESP_ERROR_CHECK(esp_wifi_scan_start(&config, true));

    uint16_t apCount = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&apCount));

    std::vector<wifi_ap_record_t> records(apCount);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apCount, records.data()));

    std::vector<ScannedNetwork> result;
    result.reserve(records.size());

    for (const auto& ap : records) {
        const size_t ssidLen = strnlen(reinterpret_cast<const char*>(ap.ssid), sizeof(ap.ssid));
        std::string ssid(reinterpret_cast<const char*>(ap.ssid), ssidLen);
        ScannedNetwork network;
        network.ssid = std::move(ssid);
        network.bssid = formatBssid(ap.bssid);
        network.rssi = ap.rssi;
        network.channel = ap.primary;
        network.auth = authToString(ap.authmode);
        network.hidden = network.ssid.empty();
        result.push_back(std::move(network));
    }

    ESP_LOGI(TAG, "Scan found %u APs", static_cast<unsigned>(result.size()));
    return result;
}

std::string WifiScanner::toJson(const std::vector<ScannedNetwork>& networks) const
{
    cJSON *arr = cJSON_CreateArray();
    for (const auto& n : networks) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "ssid", n.ssid.c_str());
        cJSON_AddStringToObject(item, "bssid", n.bssid.c_str());
        cJSON_AddNumberToObject(item, "rssi", static_cast<double>(n.rssi));
        cJSON_AddNumberToObject(item, "chan", static_cast<double>(n.channel));
        cJSON_AddStringToObject(item, "auth", n.auth.c_str());
        cJSON_AddBoolToObject(item, "hidden", n.hidden);
        cJSON_AddItemToArray(arr, item);
    }
    const std::string out = cjsonToString(arr);
    cJSON_Delete(arr);
    return out;
}

bool WifiScanner::start(uint32_t)
{
    if (!mutex_) {
        mutex_ = xSemaphoreCreateMutex();
    }
    if (!mutex_) {
        state_ = ScanState::Error;
        return false;
    }
    if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE) {
        state_ = ScanState::Error;
        return false;
    }
    state_ = ScanState::Scanning;
    xSemaphoreGive(mutex_);

    const auto networks = scanNetworks();
    const auto json = toJson(networks);

    if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE) {
        state_ = ScanState::Error;
        return false;
    }
    last_json_ = json;
    state_ = ScanState::Ready;
    xSemaphoreGive(mutex_);
    return true;
}

void WifiScanner::stop()
{
    esp_wifi_scan_stop();
    if (!mutex_) {
        state_ = ScanState::Idle;
        return;
    }
    if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE) {
        return;
    }
    state_ = ScanState::Idle;
    xSemaphoreGive(mutex_);
}

ScanState WifiScanner::state() const
{
    if (!mutex_) {
        return state_;
    }
    if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE) {
        return ScanState::Error;
    }
    const ScanState st = state_;
    xSemaphoreGive(mutex_);
    return st;
}

bool WifiScanner::getResult(std::string& out) const
{
    if (!mutex_) {
        out = last_json_;
        return state_ != ScanState::Error;
    }
    if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE) {
        out = "[]";
        return false;
    }
    out = last_json_;
    const ScanState st = state_;
    xSemaphoreGive(mutex_);
    return st != ScanState::Error;
}
