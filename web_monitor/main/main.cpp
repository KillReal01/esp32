#include "Handlers.h"
#include "LittleFs.h"
#include "LogCapture.h"
#include "WebServer.h"
#include "wifi/AccessPoint.h"
#include "wifi/WifiScanner.h"

#include "esp_log.h"

namespace {
constexpr const char *TAG = "Application";
}

extern "C" void app_main(void)
{
    AccessPointManager apManager("ESP32-AP", "esp32pass", 1, 8);
    WifiScanner scanner;
    AuthService authService("esp32_secure_token");
    DeviceService deviceService(apManager);
    WebServer webServer(apManager, scanner, deviceService, authService);

    LogCapture::instance().init();

    ESP_ERROR_CHECK(apManager.initNvs());
    ESP_ERROR_CHECK(mount_littlefs());
    ESP_ERROR_CHECK(apManager.startSoftAp());
    ESP_ERROR_CHECK(webServer.start() != nullptr ? ESP_OK : ESP_FAIL);

    ESP_LOGI(TAG, "System started");
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
