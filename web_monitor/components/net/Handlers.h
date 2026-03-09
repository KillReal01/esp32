#ifndef HANDLERS_H
#define HANDLERS_H

#include <string>
#include <string_view>

#include "esp_err.h"
#include "wifi/AccessPoint.h"

class AuthService {
public:
    explicit AuthService(std::string expectedToken);

    bool isHeaderAuthorized(std::string_view authHeader) const;
    bool isTokenValidFormat(std::string_view token) const;
    std::string validateTokenResponse(std::string_view token) const;

private:
    std::string expectedToken_;
};

class DeviceService {
public:
    explicit DeviceService(AccessPointManager& apManager);

    void reboot() const;
    std::string getSysinfoJson() const;
    std::string getClientsJson() const;
    std::string getLogs() const;
    std::string getStationsForApJson(std::string_view bssid) const;

private:
    AccessPointManager& apManager_;
};

#endif // HANDLERS_H
