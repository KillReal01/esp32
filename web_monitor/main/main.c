#include "AccessPoint.h"
#include "WebServer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_littlefs.h"
#include "esp_err.h"

static const char *TAG = "Application";

static esp_err_t mount_littlefs(void)
{
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/data",
        .partition_label = "storage",
        .format_if_mount_failed = true,
        .read_only = false,
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
 
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_littlefs_info(conf.partition_label, &total, &used);
    if (ret == ESP_OK)
        ESP_LOGI(TAG, "LittleFS mounted: total=%u bytes, used=%u bytes", total, used);
    else
        ESP_LOGW(TAG, "Failed to get LittleFS info (%s)", esp_err_to_name(ret));

    return ret;
}

void app_main(void)
{
    ESP_LOGI(TAG, "==== Application start ====");

    ESP_LOGI(TAG, "Initializing NVS...");
    if (access_point_init_nvs() == ESP_OK)
        ESP_LOGI(TAG, "NVS initialized successfully");
    else
        ESP_LOGE(TAG, "Failed to initialize NVS");

    ESP_LOGI(TAG, "Mounting LittleFS...");
    if (mount_littlefs() == ESP_OK)
        ESP_LOGI(TAG, "LittleFS mounted successfully");
    else
        ESP_LOGE(TAG, "Failed to mount LittleFS");

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
