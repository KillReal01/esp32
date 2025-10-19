#ifndef ACCESS_POINT_H
#define ACCESS_POINT_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Инициализация NVS
esp_err_t access_point_init_nvs(void);

// Запуск SoftAP
esp_err_t access_point_start_softap(void);

#ifdef __cplusplus
}
#endif

#endif
