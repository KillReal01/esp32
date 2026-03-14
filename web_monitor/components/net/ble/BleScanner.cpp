#include "ble/BleScanner.h"

#include <cstdio>
#include <cstring>

#include "cJSON.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_err.h"
#include "esp_gap_ble_api.h"
#include "esp_log.h"

namespace {
constexpr const char *TAG = "BleScanner";
constexpr uint32_t kDefaultScanSeconds = 5;
BleScanner *g_ble_scanner = nullptr;

std::string formatAddress(const uint8_t bda[6])
{
    char out[18] = {0};
    std::snprintf(out, sizeof(out), "%02X:%02X:%02X:%02X:%02X:%02X",
                  bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
    return out;
}

std::string cjsonToString(cJSON *root)
{
    if (!root) return "[]";
    char *rendered = cJSON_PrintUnformatted(root);
    if (!rendered) return "[]";
    std::string out(rendered);
    cJSON_free(rendered);
    return out;
}
} // namespace

BleScanner::BleScanner()
{
    mutex_ = xSemaphoreCreateMutex();
    done_sem_ = xSemaphoreCreateBinary();
    g_ble_scanner = this;
}

esp_err_t BleScanner::init()
{
    if (initialized_)
        return ESP_OK;

    if (esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to release classic BT memory");
    }

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_err_t err = esp_bt_controller_init(&bt_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_bt_controller_init failed: %s", esp_err_to_name(err));
        setState(ScanState::Error);
        return err;
    }

    err = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_bt_controller_enable failed: %s", esp_err_to_name(err));
        setState(ScanState::Error);
        return err;
    }

    err = esp_bluedroid_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_bluedroid_init failed: %s", esp_err_to_name(err));
        setState(ScanState::Error);
        return err;
    }

    err = esp_bluedroid_enable();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_bluedroid_enable failed: %s", esp_err_to_name(err));
        setState(ScanState::Error);
        return err;
    }

    err = esp_ble_gap_register_callback(&BleScanner::gapCallback);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ble_gap_register_callback failed: %s", esp_err_to_name(err));
        setState(ScanState::Error);
        return err;
    }

    esp_ble_scan_params_t scan_params{};
    scan_params.scan_type = BLE_SCAN_TYPE_ACTIVE;
    scan_params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
    scan_params.scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL;
    scan_params.scan_interval = 0x50;
    scan_params.scan_window = 0x30;

    err = esp_ble_gap_set_scan_params(&scan_params);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ble_gap_set_scan_params failed: %s", esp_err_to_name(err));
        setState(ScanState::Error);
        return err;
    }

    initialized_ = true;
    setState(ScanState::Idle);
    return ESP_OK;
}

bool BleScanner::start(uint32_t durationSeconds)
{
   if (!initialized_) {
        setState(ScanState::Error);
        return false;
    }
    if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE) {
        setState(ScanState::Error);
        return false;
    }
    if (state_ == ScanState::Scanning) {
        xSemaphoreGive(mutex_);
        return false;
    }
    devices_.clear();
    last_json_ = "[]";
    if (done_sem_) {
        xSemaphoreTake(done_sem_, 0);
    }
    state_ = ScanState::Scanning;
    xSemaphoreGive(mutex_);

    const uint32_t duration = durationSeconds == 0 ? kDefaultScanSeconds : durationSeconds;
    const esp_err_t err = esp_ble_gap_start_scanning(duration);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ble_gap_start_scanning failed: %s", esp_err_to_name(err));
        setState(ScanState::Error);
    }
    if (!done_sem_) {
        return false;
    }
    const uint32_t timeoutMs = (durationSeconds == 0 ? kDefaultScanSeconds : durationSeconds) * 1000 + 1000;
    if (xSemaphoreTake(done_sem_, pdMS_TO_TICKS(timeoutMs)) != pdTRUE) {
        setState(ScanState::Error);
    }
    return state_ != ScanState::Error;
}

ScanState BleScanner::getState(std::string *json) const
{
    if (!mutex_) {
        if (json) *json = "[]";
        return ScanState::Error;
    }
    if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE) {
        if (json) 
            *json = "[]";
        return ScanState::Error;
    }
    const ScanState state = state_;
    if (json) *json = last_json_;
    xSemaphoreGive(mutex_);
    return state;
}

void BleScanner::gapCallback(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    if (!g_ble_scanner)
        return;
    g_ble_scanner->handleGapEvent(event, param);
}

void BleScanner::handleGapEvent(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    if (!param)
        return;
    if (event != ESP_GAP_BLE_SCAN_RESULT_EVT)
        return;

    const auto& scan = param->scan_rst;
    if (scan.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
        BleDevice device{};
        device.address = formatAddress(scan.bda);
        device.rssi = scan.rssi;
        device.advType = static_cast<int>(scan.ble_evt_type);
        device.scanResponse = scan.scan_rsp_len > 0;

        uint8_t nameLen = 0;
        uint8_t *advData = const_cast<uint8_t*>(scan.ble_adv);
        const uint8_t *name = esp_ble_resolve_adv_data(advData, ESP_BLE_AD_TYPE_NAME_CMPL, &nameLen);
        if (!name || nameLen == 0) {
            name = esp_ble_resolve_adv_data(advData, ESP_BLE_AD_TYPE_NAME_SHORT, &nameLen);
        }
        if (name && nameLen) {
            device.name.assign(reinterpret_cast<const char*>(name), nameLen);
        }

        updateDevice(device);
    } else if (scan.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT) {
        finishScan();
    }
}

void BleScanner::updateDevice(const BleDevice& device)
{
    if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE) {
        return;
    }

    bool updated = false;
    for (auto& existing : devices_) {
        if (existing.address == device.address) {
            existing.rssi = device.rssi;
            existing.advType = device.advType;
            existing.scanResponse = device.scanResponse;
            if (!device.name.empty()) {
                existing.name = device.name;
            }
            updated = true;
            break;
        }
    }

    if (!updated) {
        devices_.push_back(device);
    }

    xSemaphoreGive(mutex_);
}

void BleScanner::finishScan()
{
    if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE) {
        return;
    }
    cJSON *arr = cJSON_CreateArray();
    for (const auto& d : devices_) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "addr", d.address.c_str());
        cJSON_AddStringToObject(item, "name", d.name.c_str());
        cJSON_AddNumberToObject(item, "rssi", static_cast<double>(d.rssi));
        cJSON_AddNumberToObject(item, "type", static_cast<double>(d.advType));
        cJSON_AddBoolToObject(item, "scanRsp", d.scanResponse);
        cJSON_AddItemToArray(arr, item);
    }
    last_json_ = cjsonToString(arr);
    cJSON_Delete(arr);
    state_ = ScanState::Ready;
    ESP_LOGI(TAG, "BLE scan complete: %u devices", static_cast<unsigned>(devices_.size()));
    xSemaphoreGive(mutex_);
    if (done_sem_) {
        xSemaphoreGive(done_sem_);
    }
}

void BleScanner::setState(ScanState state)
{
    if (!mutex_)
        return;
    if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE)
        return;
    state_ = state;
    xSemaphoreGive(mutex_);
}

void BleScanner::stop()
{
    esp_ble_gap_stop_scanning();
    setState(ScanState::Idle);
}

ScanState BleScanner::state() const
{
    return getState(nullptr);
}

bool BleScanner::getResult(std::string& out) const
{
    const ScanState st = getState(&out);
    return st != ScanState::Error;
}

std::string BleScanner::devicesToJson() const
{
    cJSON *arr = cJSON_CreateArray();
    if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE) {
        cJSON_Delete(arr);
        return "[]";
    }

    for (const auto& d : devices_) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "addr", d.address.c_str());
        cJSON_AddStringToObject(item, "name", d.name.c_str());
        cJSON_AddNumberToObject(item, "rssi", static_cast<double>(d.rssi));
        cJSON_AddNumberToObject(item, "type", static_cast<double>(d.advType));
        cJSON_AddBoolToObject(item, "scanRsp", d.scanResponse);
        cJSON_AddItemToArray(arr, item);
    }

    xSemaphoreGive(mutex_);
    const std::string out = cjsonToString(arr);
    cJSON_Delete(arr);
    return out;
}
