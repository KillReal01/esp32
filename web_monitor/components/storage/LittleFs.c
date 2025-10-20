#include "LittleFs.h"

#include "esp_littlefs.h"
#include "esp_err.h"
#include "esp_log.h"


static const char *TAG = "Storage";


esp_err_t mount_littlefs(void)
{
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/data",
        .partition_label = "storage",
        .format_if_mount_failed = false,
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
