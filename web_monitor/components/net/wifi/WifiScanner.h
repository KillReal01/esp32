#ifndef WIFI_SCANNER_H
#define WIFI_SCANNER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif


// --- wifi scan
esp_err_t device_scan_networks(char *buf, size_t len);


#ifdef __cplusplus
}
#endif

#endif
