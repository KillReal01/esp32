#ifndef HANDLERS_H
#define HANDLERS_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- system
esp_err_t device_reboot(void);
esp_err_t device_get_sysinfo(char *buf, size_t len);

// --- clients
esp_err_t device_get_clients(char *buf, size_t len);

// --- logs
esp_err_t device_get_logs(char *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif
