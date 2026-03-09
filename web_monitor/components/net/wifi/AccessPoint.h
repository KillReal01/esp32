#ifndef ACCESS_POINT_H
#define ACCESS_POINT_H

#include <string>
#include <string_view>
#include <vector>

#include "esp_err.h"
#include "esp_wifi_types.h"

struct StationInfo {
    std::string mac;
    std::string ip;
    uint16_t aid;
};

class AccessPointManager {
public:
    explicit AccessPointManager(std::string ssid, std::string password, uint8_t channel = 1, uint8_t maxConnections = 8);

    esp_err_t initNvs();
    esp_err_t startSoftAp();

    esp_err_t connectToExternalAp(std::string_view ssid, std::string_view password);
    std::vector<StationInfo> getConnectedStations() const;

    const std::string& ssid() const { return apSsid_; }

private:
    static void wifiEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

    std::string apSsid_;
    std::string apPassword_;
    uint8_t channel_;
    uint8_t maxConnections_;
};

#endif // ACCESS_POINT_H
