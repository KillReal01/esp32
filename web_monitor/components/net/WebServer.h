#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <string>

#include "Handlers.h"
#include "esp_http_server.h"
#include "scan/IScanner.h"
#include "wifi/AccessPoint.h"

class WebServer {
public:
    WebServer(
        AccessPointManager& apManager,
        IScanner& wifiScanner,
        IScanner& bleScanner,
        DeviceService& deviceService,
        AuthService& authService
    );
    httpd_handle_t start();

private:
    static esp_err_t rootGetHandler(httpd_req_t *req);
    static esp_err_t cssGetHandler(httpd_req_t *req);
    static esp_err_t jsGetHandler(httpd_req_t *req);
    static esp_err_t iconGetHandler(httpd_req_t *req);
    static esp_err_t iconPngGetHandler(httpd_req_t *req);
    static esp_err_t scanGetHandler(httpd_req_t *req);
    static esp_err_t bleScanGetHandler(httpd_req_t *req);
    static esp_err_t stationsGetHandler(httpd_req_t *req);
    static esp_err_t logsGetHandler(httpd_req_t *req);
    static esp_err_t rebootPostHandler(httpd_req_t *req);
    static esp_err_t sysinfoGetHandler(httpd_req_t *req);
    static esp_err_t connectPostHandler(httpd_req_t *req);
    static esp_err_t validateTokenPostHandler(httpd_req_t *req);
    static esp_err_t apClientsGetHandler(httpd_req_t *req);

    esp_err_t handleWiFiScan(httpd_req_t *req);
    esp_err_t handleBleScan(httpd_req_t *req);
    esp_err_t handleStations(httpd_req_t *req);
    esp_err_t handleLogs(httpd_req_t *req);
    esp_err_t handleReboot(httpd_req_t *req);
    esp_err_t handleSysinfo(httpd_req_t *req);
    esp_err_t handleConnect(httpd_req_t *req);
    esp_err_t handleValidateToken(httpd_req_t *req);
    esp_err_t handleApClients(httpd_req_t *req);

    bool ensureAuthorized(httpd_req_t *req);
    static WebServer* fromReq(httpd_req_t *req);
    static esp_err_t serveFile(httpd_req_t *req, const char *filepath, const char *content_type);
    static std::string getHeader(httpd_req_t *req, const char* name);
    static std::string getBody(httpd_req_t *req);
    static std::string getQueryParam(httpd_req_t *req, const char* key);

    void registerUris(httpd_handle_t server);
    void registerUri(httpd_handle_t server, const char *uri, httpd_method_t method, esp_err_t (*handler)(httpd_req_t *));

    AccessPointManager& apManager_;
    IScanner& wifiScanner_;
    IScanner& bleScanner_;
    DeviceService& deviceService_;
    AuthService& authService_;
};

#endif // WEBSERVER_H
