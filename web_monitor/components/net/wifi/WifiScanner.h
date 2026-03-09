#ifndef WIFI_SCANNER_H
#define WIFI_SCANNER_H

#include "esp_err.h"
#include <string>


esp_err_t device_scan_networks(std::string& out);


#endif // WIFI_SCANNER_H
