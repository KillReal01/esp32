#include "WebServer.h"
#include "Handlers.h"
#include "wifi/WifiScanner.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/inet.h"

static const char *TAG = "WebServer";

static const char *get_client_ip(httpd_req_t *req)
{
    static char ip_str[INET6_ADDRSTRLEN] = {0};
    sockaddr_in6 addr{};
    socklen_t len = sizeof(addr);

    if (getpeername(httpd_req_to_sockfd(req), reinterpret_cast<sockaddr *>(&addr), &len) < 0) {
        std::strcpy(ip_str, "unknown");
        return ip_str;
    }

    if (IN6_IS_ADDR_V4MAPPED(&addr.sin6_addr)) {
        in_addr ipv4{};
        std::memcpy(&ipv4, &addr.sin6_addr.s6_addr[12], sizeof(ipv4));
        inet_ntop(AF_INET, &ipv4, ip_str, sizeof(ip_str));
    } else if (addr.sin6_family == AF_INET6) {
        inet_ntop(AF_INET6, &addr.sin6_addr, ip_str, sizeof(ip_str));
    } else {
        std::strcpy(ip_str, "invalid");
    }

    return ip_str;
}

static void log_request(httpd_req_t *req)
{
    ESP_LOGI(TAG, "[Request] %s %s from %s",
             http_method_str(req->method), req->uri, get_client_ip(req));
}

static esp_err_t serve_file(httpd_req_t *req, const char *filepath, const char *content_type)
{
    FILE *f = std::fopen(filepath, "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s", filepath);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, content_type);
    char buf[512];
    size_t read_bytes;
    while ((read_bytes = std::fread(buf, 1, sizeof(buf), f)) > 0) {
        httpd_resp_send_chunk(req, buf, read_bytes);
    }
    std::fclose(f);
    httpd_resp_send_chunk(req, nullptr, 0);
    return ESP_OK;
}

static esp_err_t root_get_handler(httpd_req_t *req) { return serve_file(req, "/data/index.html", "text/html"); }
static esp_err_t css_get_handler(httpd_req_t *req) { return serve_file(req, "/data/style.css", "text/css"); }
static esp_err_t js_get_handler(httpd_req_t *req) { return serve_file(req, "/data/script.js", "application/javascript"); }
static esp_err_t icon_get_handler(httpd_req_t *req) { return serve_file(req, "/data/favicon.png", "image/x-icon"); }

typedef struct {
    httpd_req_t *req;
} scan_task_param_t;

static void wifi_scan_worker(void *arg)
{
    auto *param = static_cast<scan_task_param_t *>(arg);
    char buf[4096];

    if (device_scan_networks(buf, sizeof(buf)) != ESP_OK) {
        ESP_LOGE(TAG, "WiFi scan failed");
        httpd_resp_send_500(param->req);
        std::free(param);
        return;
    }

    httpd_resp_set_type(param->req, "application/json");
    httpd_resp_send(param->req, buf, HTTPD_RESP_USE_STRLEN);
    std::free(param);
}

static esp_err_t scan_get_handler(httpd_req_t *req)
{
    log_request(req);

    auto *param = static_cast<scan_task_param_t *>(std::malloc(sizeof(scan_task_param_t)));
    if (!param) {
        return httpd_resp_send_500(req);
    }

    param->req = req;

    if (httpd_queue_work(req->handle, wifi_scan_worker, param) != ESP_OK) {
        std::free(param);
        return httpd_resp_send_500(req);
    }

    return ESP_OK;
}

static esp_err_t stations_get_handler(httpd_req_t *req)
{
    log_request(req);
    char buf[256];
    device_get_clients(buf, sizeof(buf));

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t logs_get_handler(httpd_req_t *req)
{
    log_request(req);
    char buf[1024];
    device_get_logs(buf, sizeof(buf));

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t reboot_post_handler(httpd_req_t *req)
{
    log_request(req);
    const char *resp = "{\"status\":\"ok\"}";

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

    device_reboot();
    return ESP_OK;
}

static esp_err_t sysinfo_get_handler(httpd_req_t *req)
{
    log_request(req);
    char buf[256];
    device_get_sysinfo(buf, sizeof(buf));

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static void register_uri(httpd_handle_t server, const char *uri, httpd_method_t method, esp_err_t (*handler)(httpd_req_t *))
{
    httpd_uri_t cfg{};
    cfg.uri = uri;
    cfg.method = method;
    cfg.handler = handler;
    cfg.user_ctx = nullptr;
    httpd_register_uri_handler(server, &cfg);
}

static void register_uris(httpd_handle_t server)
{
    register_uri(server, "/", HTTP_GET, root_get_handler);
    register_uri(server, "/style.css", HTTP_GET, css_get_handler);
    register_uri(server, "/script.js", HTTP_GET, js_get_handler);
    register_uri(server, "/favicon.ico", HTTP_GET, icon_get_handler);
    register_uri(server, "/api/scan", HTTP_GET, scan_get_handler);
    register_uri(server, "/api/stations", HTTP_GET, stations_get_handler);
    register_uri(server, "/api/logs", HTTP_GET, logs_get_handler);
    register_uri(server, "/api/reboot", HTTP_POST, reboot_post_handler);
    register_uri(server, "/api/sysinfo", HTTP_GET, sysinfo_get_handler);
}

httpd_handle_t webserver_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 12;

    httpd_handle_t server = nullptr;
    if (httpd_start(&server, &config) == ESP_OK) {
        register_uris(server);
        ESP_LOGI(TAG, "Webserver started");
    } else {
        ESP_LOGE(TAG, "Failed to start webserver");
    }

    return server;
}
