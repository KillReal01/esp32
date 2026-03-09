#include "wifi/WifiScanner.h"

#include <cstdio>
#include <cstring>
#include <utility>

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

std::string escapeJson(const std::string& input)
{
    std::string out;
    out.reserve(input.size() + 8);
    for (char c : input) {
        if (c == '"' || c == '\\') {
            out.push_back('\\');
        }
        out.push_back(c);
    }
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
    std::string json;
    json.reserve(networks.size() * 88);
    json += "[";

    for (size_t i = 0; i < networks.size(); ++i) {
        const auto& n = networks[i];
        json += "{\"ssid\":\"" + escapeJson(n.ssid) + "\"";
        json += ",\"bssid\":\"" + n.bssid + "\"";
        json += ",\"rssi\":" + std::to_string(n.rssi);
        json += ",\"chan\":" + std::to_string(n.channel);
        json += ",\"auth\":\"" + n.auth + "\"";
        json += ",\"hidden\":" + std::string(n.hidden ? "true" : "false");
        json += "}";
        if (i + 1 < networks.size()) json += ",";
    }

    json += "]";
    return json;
}
