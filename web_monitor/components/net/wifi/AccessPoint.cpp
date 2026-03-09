#include "wifi/AccessPoint.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <utility>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "lwip/inet.h"
#include "nvs_flash.h"

namespace {
constexpr const char *TAG = "AccessPoint";

std::string formatMac(const uint8_t mac[6])
{
    char out[18] = {0};
    std::snprintf(out, sizeof(out), "%02X:%02X:%02X:%02X:%02X:%02X",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return out;
}
} // namespace

AccessPointManager::AccessPointManager(std::string ssid, std::string password, uint8_t channel, uint8_t maxConnections)
    : apSsid_(std::move(ssid)), apPassword_(std::move(password)), channel_(channel), maxConnections_(maxConnections)
{
}

void AccessPointManager::wifiEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        auto *event = static_cast<wifi_event_ap_staconnected_t *>(event_data);
        ESP_LOGI(TAG, "STA joined: " MACSTR ", AID=%d", MAC2STR(event->mac), event->aid);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        auto *event = static_cast<wifi_event_ap_stadisconnected_t *>(event_data);
        ESP_LOGI(TAG, "STA left: " MACSTR ", AID=%d", MAC2STR(event->mac), event->aid);
    }
}

esp_err_t AccessPointManager::initNvs()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    return ESP_OK;
}

esp_err_t AccessPointManager::startSoftAp()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &AccessPointManager::wifiEventHandler,
                                                        nullptr, nullptr));

    wifi_config_t wifiConfig{};
    std::memcpy(wifiConfig.ap.ssid, apSsid_.data(), apSsid_.size());
    std::memcpy(wifiConfig.ap.password, apPassword_.data(), apPassword_.size());
    wifiConfig.ap.ssid_len = apSsid_.size();
    wifiConfig.ap.channel = channel_;
    wifiConfig.ap.max_connection = maxConnections_;
    wifiConfig.ap.authmode = apPassword_.empty() ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifiConfig));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "SoftAP started: %s", apSsid_.c_str());
    return ESP_OK;
}

esp_err_t AccessPointManager::connectToExternalAp(std::string_view ssid, std::string_view password)
{
    if (ssid.empty()) {
        return ESP_ERR_INVALID_ARG;
    }

    wifi_config_t staCfg{};
    std::memcpy(staCfg.sta.ssid, ssid.data(), std::min(ssid.size(), sizeof(staCfg.sta.ssid) - 1));
    std::memcpy(staCfg.sta.password, password.data(), std::min(password.size(), sizeof(staCfg.sta.password) - 1));
    staCfg.sta.threshold.authmode = WIFI_AUTH_OPEN;
    staCfg.sta.pmf_cfg.capable = true;
    staCfg.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &staCfg));
    ESP_LOGI(TAG, "Connecting STA to %.*s", static_cast<int>(ssid.size()), ssid.data());
    return esp_wifi_connect();
}

std::vector<StationInfo> AccessPointManager::getConnectedStations() const
{
    std::vector<StationInfo> stations;

    wifi_sta_list_t staList{};
    if (esp_wifi_ap_get_sta_list(&staList) != ESP_OK) {
        return stations;
    }

    for (int i = 0; i < staList.num; ++i) {
        const wifi_sta_info_t& sta = staList.sta[i];
        StationInfo info;
        info.mac = formatMac(sta.mac);
        info.ip = "-";
        info.aid = static_cast<uint16_t>(i + 1);
        stations.push_back(std::move(info));
    }

    return stations;
}
