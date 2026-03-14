#ifndef BLE_SCANNER_H
#define BLE_SCANNER_H

#include <string>
#include <vector>

#include "esp_err.h"
#include "esp_gap_ble_api.h"
#include "scan/IScanner.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

struct BleDevice {
    std::string address;
    std::string name;
    int rssi;
    int advType;
    bool scanResponse;
};

class BleScanner final: public IScanner {
public:
    BleScanner();
    esp_err_t init();
    ScanState getState(std::string *json) const;
    bool start(uint32_t durationSeconds) override;
    void stop() override;
    ScanState state() const override;
    bool getResult(std::string& out) const override;

private:
    static void gapCallback(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
    void handleGapEvent(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
    void updateDevice(const BleDevice& device);
    void finishScan();
    void setState(ScanState state);
    std::string devicesToJson() const;

    mutable SemaphoreHandle_t mutex_{};
    SemaphoreHandle_t done_sem_{};
    ScanState state_{ScanState::Idle};
    std::vector<BleDevice> devices_;
    std::string last_json_{"[]"};
    bool initialized_{false};
};

#endif // BLE_SCANNER_H
