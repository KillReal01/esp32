#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

void app_main(void)
{
    const char *TAG = "HELLO";
    ESP_LOGI(TAG, "ESP32 test program started");

    while (1) {
        ESP_LOGI(TAG, "Hello, KillReal! Free heap: %ld bytes", esp_get_free_heap_size());
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
