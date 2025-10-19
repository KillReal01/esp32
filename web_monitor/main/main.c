#include "AccessPoint.h"
#include "WebServer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "Application";

void app_main(void)
{
    ESP_LOGI(TAG, "==== Application start ====");

    ESP_LOGI(TAG, "Initializing NVS...");
    if (access_point_init_nvs() == ESP_OK)
        ESP_LOGI(TAG, "NVS initialized successfully");
    else
        ESP_LOGE(TAG, "Failed to initialize NVS");

    ESP_LOGI(TAG, "Starting SoftAP...");
    if (access_point_start_softap() == ESP_OK)
        ESP_LOGI(TAG, "SoftAP started successfully");
    else
        ESP_LOGE(TAG, "Failed to start SoftAP");

    ESP_LOGI(TAG, "Starting WebServer...");
    httpd_handle_t server = webserver_start();
    if (server != NULL)
        ESP_LOGI(TAG, "WebServer started successfully");
    else
        ESP_LOGE(TAG, "Failed to start WebServer");

    ESP_LOGI(TAG, "Entering main loop");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
