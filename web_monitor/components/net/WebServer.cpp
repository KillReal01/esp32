#include "WebServer.h"

#include <cstdio>

#include "esp_log.h"
#include "lwip/inet.h"

namespace {
constexpr const char *TAG = "WebServer";
constexpr size_t kMaxBody = 512;
}

WebServer::WebServer(AccessPointManager& apManager, WifiScanner& scanner, DeviceService& deviceService, AuthService& authService)
    : apManager_(apManager), scanner_(scanner), deviceService_(deviceService), authService_(authService)
{
}

WebServer* WebServer::fromReq(httpd_req_t *req)
{
    return static_cast<WebServer*>(req->user_ctx);
}

esp_err_t WebServer::serveFile(httpd_req_t *req, const char *filepath, const char *contentType)
{
    FILE *f = std::fopen(filepath, "r");
    if (!f) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, contentType);
    char buffer[512];
    size_t readBytes = 0;
    while ((readBytes = std::fread(buffer, 1, sizeof(buffer), f)) > 0) {
        httpd_resp_send_chunk(req, buffer, readBytes);
    }
    std::fclose(f);
    httpd_resp_send_chunk(req, nullptr, 0);
    return ESP_OK;
}

std::string WebServer::getHeader(httpd_req_t *req, const char* name)
{
    const size_t len = httpd_req_get_hdr_value_len(req, name);
    if (len == 0) return {};

    std::string value(len + 1, '\0');
    if (httpd_req_get_hdr_value_str(req, name, value.data(), value.size()) != ESP_OK) {
        return {};
    }

    value.resize(len);
    return value;
}

std::string WebServer::getBody(httpd_req_t *req)
{
    if (req->content_len <= 0 || req->content_len > static_cast<int>(kMaxBody)) {
        return {};
    }

    std::string body(req->content_len, '\0');
    int received = httpd_req_recv(req, body.data(), body.size());
    if (received <= 0) {
        return {};
    }
    body.resize(received);
    return body;
}

std::string WebServer::getQueryParam(httpd_req_t *req, const char* key)
{
    const size_t queryLen = httpd_req_get_url_query_len(req);
    if (queryLen == 0) return {};

    std::string query(queryLen + 1, '\0');
    if (httpd_req_get_url_query_str(req, query.data(), query.size()) != ESP_OK) {
        return {};
    }

    char value[64] = {0};
    if (httpd_query_key_value(query.c_str(), key, value, sizeof(value)) != ESP_OK) {
        return {};
    }
    return value;
}

bool WebServer::ensureAuthorized(httpd_req_t *req)
{
    const std::string authHeader = getHeader(req, "Authorization");
    if (!authService_.isHeaderAuthorized(authHeader)) {
        httpd_resp_set_status(req, "401 Unauthorized");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"error\":\"unauthorized\"}", HTTPD_RESP_USE_STRLEN);
        return false;
    }
    return true;
}

esp_err_t WebServer::rootGetHandler(httpd_req_t *req) { return serveFile(req, "/data/index.html", "text/html"); }
esp_err_t WebServer::cssGetHandler(httpd_req_t *req) { return serveFile(req, "/data/style.css", "text/css"); }
esp_err_t WebServer::jsGetHandler(httpd_req_t *req) { return serveFile(req, "/data/script.js", "application/javascript"); }
esp_err_t WebServer::iconGetHandler(httpd_req_t *req) { return serveFile(req, "/data/favicon.png", "image/x-icon"); }

esp_err_t WebServer::scanGetHandler(httpd_req_t *req) { return fromReq(req)->handleScan(req); }
esp_err_t WebServer::stationsGetHandler(httpd_req_t *req) { return fromReq(req)->handleStations(req); }
esp_err_t WebServer::logsGetHandler(httpd_req_t *req) { return fromReq(req)->handleLogs(req); }
esp_err_t WebServer::rebootPostHandler(httpd_req_t *req) { return fromReq(req)->handleReboot(req); }
esp_err_t WebServer::sysinfoGetHandler(httpd_req_t *req) { return fromReq(req)->handleSysinfo(req); }
esp_err_t WebServer::connectPostHandler(httpd_req_t *req) { return fromReq(req)->handleConnect(req); }
esp_err_t WebServer::validateTokenPostHandler(httpd_req_t *req) { return fromReq(req)->handleValidateToken(req); }
esp_err_t WebServer::apClientsGetHandler(httpd_req_t *req) { return fromReq(req)->handleApClients(req); }

esp_err_t WebServer::handleScan(httpd_req_t *req)
{
    if (!ensureAuthorized(req)) return ESP_OK;
    const auto networks = scanner_.scanNetworks();
    const auto payload = scanner_.toJson(networks);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, payload.c_str(), payload.size());
}

esp_err_t WebServer::handleStations(httpd_req_t *req)
{
    if (!ensureAuthorized(req)) return ESP_OK;
    const auto payload = deviceService_.getClientsJson();
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, payload.c_str(), payload.size());
}

esp_err_t WebServer::handleLogs(httpd_req_t *req)
{
    if (!ensureAuthorized(req)) return ESP_OK;
    const auto payload = deviceService_.getLogs();
    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, payload.c_str(), payload.size());
}

esp_err_t WebServer::handleReboot(httpd_req_t *req)
{
    if (!ensureAuthorized(req)) return ESP_OK;
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"ok\"}", HTTPD_RESP_USE_STRLEN);
    deviceService_.reboot();
    return ESP_OK;
}

esp_err_t WebServer::handleSysinfo(httpd_req_t *req)
{
    if (!ensureAuthorized(req)) return ESP_OK;
    const auto payload = deviceService_.getSysinfoJson();
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, payload.c_str(), payload.size());
}

esp_err_t WebServer::handleConnect(httpd_req_t *req)
{
    if (!ensureAuthorized(req)) return ESP_OK;
    const std::string body = getBody(req);

    const auto sPos = body.find("\"ssid\":\"");
    const auto pPos = body.find("\"password\":\"");
    if (sPos == std::string::npos || pPos == std::string::npos) {
        return httpd_resp_send_400(req);
    }

    const auto ssidStart = sPos + 8;
    const auto ssidEnd = body.find('"', ssidStart);
    const auto passStart = pPos + 12;
    const auto passEnd = body.find('"', passStart);
    const std::string ssid = body.substr(ssidStart, ssidEnd - ssidStart);
    const std::string pass = body.substr(passStart, passEnd - passStart);

    const esp_err_t err = apManager_.connectToExternalAp(ssid, pass);
    httpd_resp_set_type(req, "application/json");
    if (err != ESP_OK) {
        return httpd_resp_send(req, "{\"status\":\"error\"}", HTTPD_RESP_USE_STRLEN);
    }
    return httpd_resp_send(req, "{\"status\":\"connecting\"}", HTTPD_RESP_USE_STRLEN);
}

esp_err_t WebServer::handleValidateToken(httpd_req_t *req)
{
    const std::string body = getBody(req);
    const auto tokenPos = body.find("\"token\":\"");
    if (tokenPos == std::string::npos) return httpd_resp_send_400(req);

    const auto tokenStart = tokenPos + 9;
    const auto tokenEnd = body.find('"', tokenStart);
    const std::string token = body.substr(tokenStart, tokenEnd - tokenStart);

    const auto payload = authService_.validateTokenResponse(token);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, payload.c_str(), payload.size());
}

esp_err_t WebServer::handleApClients(httpd_req_t *req)
{
    if (!ensureAuthorized(req)) return ESP_OK;
    const std::string bssid = getQueryParam(req, "bssid");
    const auto payload = deviceService_.getStationsForApJson(bssid);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, payload.c_str(), payload.size());
}

void WebServer::registerUri(httpd_handle_t server, const char *uri, httpd_method_t method, esp_err_t (*handler)(httpd_req_t *))
{
    httpd_uri_t cfg{};
    cfg.uri = uri;
    cfg.method = method;
    cfg.handler = handler;
    cfg.user_ctx = this;
    httpd_register_uri_handler(server, &cfg);
}

void WebServer::registerUris(httpd_handle_t server)
{
    registerUri(server, "/", HTTP_GET, rootGetHandler);
    registerUri(server, "/style.css", HTTP_GET, cssGetHandler);
    registerUri(server, "/script.js", HTTP_GET, jsGetHandler);
    registerUri(server, "/favicon.ico", HTTP_GET, iconGetHandler);
    registerUri(server, "/api/scan", HTTP_GET, scanGetHandler);
    registerUri(server, "/api/stations", HTTP_GET, stationsGetHandler);
    registerUri(server, "/api/logs", HTTP_GET, logsGetHandler);
    registerUri(server, "/api/reboot", HTTP_POST, rebootPostHandler);
    registerUri(server, "/api/sysinfo", HTTP_GET, sysinfoGetHandler);
    registerUri(server, "/api/connect", HTTP_POST, connectPostHandler);
    registerUri(server, "/api/token/validate", HTTP_POST, validateTokenPostHandler);
    registerUri(server, "/api/ap-clients", HTTP_GET, apClientsGetHandler);
}

httpd_handle_t WebServer::start()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;

    httpd_handle_t server = nullptr;
    if (httpd_start(&server, &config) == ESP_OK) {
        registerUris(server);
        ESP_LOGI(TAG, "Webserver started");
    }
    return server;
}
