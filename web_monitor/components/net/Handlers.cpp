#include "Handlers.h"

#include <cinttypes>
#include <cstdio>

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

std::string jsonEscape(std::string_view input)
{
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
        if (c == '"' || c == '\\') {
            out.push_back('\\');
        }
        out.push_back(c);
    }
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
    return std::string("{\"valid\":") + (authorized ? "true" : "false") +
           ",\"formatOk\":" + (formatOk ? "true" : "false") +
           ",\"reason\":\"" + reason + "\"}";
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

    char buffer[320] = {0};
    std::snprintf(buffer, sizeof(buffer),
                  "{\"firmware\":\"v2.0.0\",\"uptimeSec\":%" PRIu32 ",\"freeHeapKb\":%" PRIu32
                  ",\"flashKb\":%" PRIu32 ",\"healthScore\":%d}",
                  uptimeS, freeHeap / 1024, flashSize / 1024, healthScore);
    return buffer;
}

std::string DeviceService::getClientsJson() const
{
    const auto stations = apManager_.getConnectedStations();
    std::string json = "[";

    for (size_t i = 0; i < stations.size(); ++i) {
        const auto& sta = stations[i];
        json += "{\"mac\":\"" + sta.mac + "\",\"ip\":\"" + sta.ip + "\",\"aid\":" + std::to_string(sta.aid) + "}";
        if (i + 1 < stations.size()) json += ",";
    }

    json += "]";
    return json;
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
    std::string json = "{\"apBssid\":\"" + jsonEscape(bssid) + "\",\"count\":" + std::to_string(stations.size()) + ",\"stations\":";

    json += "[";
    for (size_t i = 0; i < stations.size(); ++i) {
        json += "{\"mac\":\"" + stations[i].mac + "\",\"aid\":" + std::to_string(stations[i].aid) + "}";
        if (i + 1 < stations.size()) json += ",";
    }
    json += "]}";
    return json;
}
