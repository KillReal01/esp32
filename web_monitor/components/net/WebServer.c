#include "WebServer.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/inet.h"


static const char *TAG = "WebServer";


static const char *get_client_ip(httpd_req_t *req)
{
    static char ip_str[INET6_ADDRSTRLEN] = {0};
    struct sockaddr_in6 addr;
    socklen_t len = sizeof(addr);

    if (getpeername(httpd_req_to_sockfd(req), (struct sockaddr *)&addr, &len) < 0) {
        strcpy(ip_str, "unknown");
        return ip_str;
    }

    if (IN6_IS_ADDR_V4MAPPED(&addr.sin6_addr)) {
        struct in_addr ipv4;
        memcpy(&ipv4, &addr.sin6_addr.s6_addr[12], sizeof(ipv4));
        inet_ntop(AF_INET, &ipv4, ip_str, sizeof(ip_str));
    } else if (addr.sin6_family == AF_INET6) {
        inet_ntop(AF_INET6, &addr.sin6_addr, ip_str, sizeof(ip_str));
    } else {
        strcpy(ip_str, "invalid");
    }

    return ip_str;
}

static void log_request(httpd_req_t *req)
{
    const char *method = http_method_str(req->method);
    const char *uri = req->uri;
    const char *client_ip = get_client_ip(req);

    ESP_LOGI(TAG, "[Request] %s %s from %s", method, uri, client_ip);
}

static esp_err_t serve_file(httpd_req_t *req, const char *filepath, const char *content_type)
{
    FILE *f = fopen(filepath, "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s", filepath);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, content_type);

    char buf[512];
    size_t read_bytes;
    while ((read_bytes = fread(buf, 1, sizeof(buf), f)) > 0) {
        httpd_resp_send_chunk(req, buf, read_bytes);
    }
    fclose(f);

    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t root_get_handler(httpd_req_t *req)
{
    return serve_file(req, "/data/index.html", "text/html");
}

static esp_err_t css_get_handler(httpd_req_t *req)
{
    return serve_file(req, "/data/style.css", "text/css");
}

static esp_err_t js_get_handler(httpd_req_t *req)
{
    return serve_file(req, "/data/script.js", "application/javascript");
}

// --- /api/scan
static esp_err_t scan_get_handler(httpd_req_t *req)
{
    log_request(req);

    const char *resp = "[{\"ssid\":\"HomeNetwork\",\"rssi\":-42,\"chan\":6},"
                       "{\"ssid\":\"CafeFreeWiFi\",\"rssi\":-78,\"chan\":11},"
                       "{\"ssid\":\"ESP32-Setup\",\"rssi\":-33,\"chan\":1}]";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// --- /api/stations
static esp_err_t stations_get_handler(httpd_req_t *req)
{
    log_request(req);

    const char *resp = "[{\"mac\":\"AA:BB:CC:11:22:33\",\"ip\":\"192.168.4.2\",\"last\":\"5s\"},"
                       "{\"mac\":\"DE:AD:BE:EF:00:01\",\"ip\":\"192.168.4.3\",\"last\":\"23s\"}]";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// --- /api/logs
static esp_err_t logs_get_handler(httpd_req_t *req)
{
    log_request(req);

    const char *resp = "2025-10-19 12:00:00 System start\n"
                       "2025-10-19 12:00:03 WiFi AP started\n"
                       "2025-10-19 12:01:02 Scan completed\n";
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// --- /api/reboot
static esp_err_t reboot_post_handler(httpd_req_t *req)
{
    log_request(req);

    // Здесь можно вставить вызов esp_restart() или просто заглушку
    ESP_LOGI(TAG, "Reboot requested (stub)");
    const char *resp = "{\"status\":\"ok\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// --- /api/sysinfo
static esp_err_t sysinfo_get_handler(httpd_req_t *req)
{
    log_request(req);

    const char *resp = "{\"firmware\":\"v1.0.0-demo\",\"uptime\":\"00:12:34\","
                       "\"freeHeap\":\"48 KB\",\"flash\":\"4 MB\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static void register_uris(httpd_handle_t server)
{
    httpd_uri_t uris[] = {
        {"/", HTTP_GET, root_get_handler, NULL},
        {"/style.css", HTTP_GET, css_get_handler, NULL},
        {"/script.js", HTTP_GET, js_get_handler, NULL},
        {"/api/scan", HTTP_GET, scan_get_handler, NULL},
        {"/api/stations", HTTP_GET, stations_get_handler, NULL},
        {"/api/logs", HTTP_GET, logs_get_handler, NULL},
        {"/api/reboot", HTTP_POST, reboot_post_handler, NULL},
        {"/api/sysinfo", HTTP_GET, sysinfo_get_handler, NULL},
    };

    for (int i = 0; i < sizeof(uris)/sizeof(uris[0]); i++) {
        httpd_register_uri_handler(server, &uris[i]);
    }
}

httpd_handle_t webserver_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        register_uris(server);
        ESP_LOGI(TAG, "Webserver started");
    } else {
        ESP_LOGE(TAG, "Failed to start webserver");
    }

    return server;
}
