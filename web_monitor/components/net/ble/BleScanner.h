#ifndef BLE_SCANNER_H
#define BLE_SCANNER_H

#include <string>
#include <vector>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

struct BleDevice {
    std::string address;
    std::string name;
    int rssi;
    int advType;
    bool scanResponse;
};

class BleScanner {
public:
    enum class State : uint8_t { Idle, Scanning, Ready, Error };

    BleScanner();
    bool init();
    void startScanAsync(uint32_t durationSeconds);
    State getState(std::string *json) const;

private:
    static void gapCallback(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
    void handleGapEvent(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
    void updateDevice(const BleDevice& device);
    void finishScan();
    void setState(State state);
    std::string devicesToJson() const;

    static BleScanner *instance_;
    mutable SemaphoreHandle_t mutex_{};
    State state_{State::Idle};
    std::vector<BleDevice> devices_;
    std::string last_json_{"[]"};
    bool initialized_{false};
};

#endif // BLE_SCANNER_H
