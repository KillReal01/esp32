#include "Handlers.h"

#include "cJSON.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"

namespace {
constexpr const char *TAG = "DeviceService";

std::string extractBearerToken(std::string_view authHeader)
{
    constexpr std::string_view kPrefix = "Bearer ";
    if (authHeader.size() < kPrefix.size() || authHeader.substr(0, kPrefix.size()) != kPrefix) {
        return {};
    }
    return std::string(authHeader.substr(kPrefix.size()));
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

AuthService::AuthService(std::string expectedToken) : expectedToken_(std::move(expectedToken)) {}

bool AuthService::isTokenValidFormat(std::string_view token) const
{
    if (token.size() < 12 || token.size() > 64) {
        return false;
    }

    for (char c : token) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
        if (!ok) {
            return false;
        }
    }

    return true;
}

bool AuthService::isHeaderAuthorized(std::string_view authHeader) const
{
    const std::string token = extractBearerToken(authHeader);
    return isTokenValidFormat(token) && token == expectedToken_;
}

std::string AuthService::validateTokenResponse(std::string_view token) const
{
    const bool formatOk = isTokenValidFormat(token);
    const bool authorized = formatOk && token == expectedToken_;
    std::string reason = authorized ? "ok" : (formatOk ? "invalid_credentials" : "invalid_format");

    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "valid", authorized);
    cJSON_AddBoolToObject(root, "formatOk", formatOk);
    cJSON_AddStringToObject(root, "reason", reason.c_str());
    const std::string out = cjsonToString(root);
    cJSON_Delete(root);
    return out;
}

DeviceService::DeviceService(AccessPointManager& apManager) : apManager_(apManager) {}

void DeviceService::reboot() const
{
    ESP_LOGW(TAG, "Reboot requested");
    esp_restart();
}

std::string DeviceService::getSysinfoJson() const
{
    const uint64_t uptimeUs = esp_timer_get_time();
    const uint32_t uptimeS = uptimeUs / 1000000ULL;

    const uint32_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    uint32_t flashSize = 0;
    esp_flash_get_size(nullptr, &flashSize);

    const int healthScore = static_cast<int>((freeHeap / 1024) > 120 ? 95 : 70);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "firmware", "v2.0.0");
    cJSON_AddNumberToObject(root, "uptimeSec", static_cast<double>(uptimeS));
    cJSON_AddNumberToObject(root, "freeHeapKb", static_cast<double>(freeHeap / 1024));
    cJSON_AddNumberToObject(root, "flashKb", static_cast<double>(flashSize / 1024));
    cJSON_AddNumberToObject(root, "healthScore", static_cast<double>(healthScore));
    const std::string out = cjsonToString(root);
    cJSON_Delete(root);
    return out;
}

std::string DeviceService::getClientsJson() const
{
    const auto stations = apManager_.getConnectedStations();
    cJSON *arr = cJSON_CreateArray();
    for (const auto& sta : stations) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "mac", sta.mac.c_str());
        cJSON_AddStringToObject(item, "ip", sta.ip.c_str());
        cJSON_AddNumberToObject(item, "aid", static_cast<double>(sta.aid));
        cJSON_AddItemToArray(arr, item);
    }
    const std::string out = cjsonToString(arr);
    cJSON_Delete(arr);
    return out;
}

std::string DeviceService::getLogs() const
{
    return "2026-03-09 10:00:00 System started\n"
           "2026-03-09 10:00:04 SoftAP active\n"
           "2026-03-09 10:00:08 Business rule: health monitored\n";
}

std::string DeviceService::getStationsForApJson(std::string_view bssid) const
{
    const auto stations = apManager_.getConnectedStations();
    const std::string bssidStr(bssid);
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "apBssid", bssidStr.c_str());
    cJSON_AddNumberToObject(root, "count", static_cast<double>(stations.size()));
    cJSON *arr = cJSON_AddArrayToObject(root, "stations");
    for (const auto& sta : stations) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "mac", sta.mac.c_str());
        cJSON_AddNumberToObject(item, "aid", static_cast<double>(sta.aid));
        cJSON_AddItemToArray(arr, item);
    }
    const std::string out = cjsonToString(root);
    cJSON_Delete(root);
    return out;
}
