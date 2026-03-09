#ifndef ACCESS_POINT_H
#define ACCESS_POINT_H

#include "esp_err.h"


esp_err_t access_point_init_nvs(void);
esp_err_t access_point_start_softap(void);


#endif // ACCESS_POINT_H
