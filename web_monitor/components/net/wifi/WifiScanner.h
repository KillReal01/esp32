#ifndef WIFI_SCANNER_H
#define WIFI_SCANNER_H

#include <string>
#include <vector>

#include "esp_err.h"
#include "scan/IScanner.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

struct ScannedNetwork {
    std::string ssid;
    std::string bssid;
    int rssi;
    int channel;
    std::string auth;
    bool hidden;
};

class WifiScanner final : public IScanner {
public:
    std::vector<ScannedNetwork> scanNetworks() const;
    std::string toJson(const std::vector<ScannedNetwork>& networks) const;
    bool start(uint32_t durationSeconds) override;
    void stop() override;
    ScanState state() const override;
    bool getResult(std::string& out) const override;

private:
    mutable SemaphoreHandle_t mutex_{};
    ScanState state_{ScanState::Idle};
    std::string last_json_{"[]"};
};

#endif // WIFI_SCANNER_H
