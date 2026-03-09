#ifndef WIFI_SCANNER_H
#define WIFI_SCANNER_H

#include <string>
#include <vector>

#include "esp_err.h"

struct ScannedNetwork {
    std::string ssid;
    std::string bssid;
    int rssi;
    int channel;
    std::string auth;
    bool hidden;
};

class WifiScanner {
public:
    std::vector<ScannedNetwork> scanNetworks() const;
    std::string toJson(const std::vector<ScannedNetwork>& networks) const;
};

#endif // WIFI_SCANNER_H
