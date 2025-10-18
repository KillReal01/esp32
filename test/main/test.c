#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"
#include "esp_netif.h"

#define WIFI_SSID "KillReal_01"
#define WIFI_PASS "killreal2002"
#define PORT 8080
#define RX_BUF_SIZE 128

static const char *TAG = "WIFI_SERVER";
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

// Обработчик события IP
static void got_ip_handler(void* arg, esp_event_base_t event_base,
                           int32_t event_id, void* event_data)
{
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    ESP_LOGI(TAG, "Connected! IP:" IPSTR, IP2STR(&event->ip_info.ip));
    xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
}

// Обработчик событий Wi-Fi
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected. Reconnecting...");
        esp_wifi_connect();
    }
}

static void wifi_init(void)
{
    wifi_event_group = xEventGroupCreate();

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &wifi_event_handler,
                                        NULL,
                                        &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT,
                                        IP_EVENT_STA_GOT_IP,
                                        &got_ip_handler,
                                        NULL,
                                        &instance_got_ip);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
}

static void tcp_server_task(void *pvParameters)
{
    // Ждём подключения к Wi-Fi
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT,
                        pdFALSE, pdTRUE, portMAX_DELAY);

    ESP_LOGI(TAG, "Starting TCP server...");

    char rx_buffer[RX_BUF_SIZE];
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);
    bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(listen_sock, 1);

    ESP_LOGI(TAG, "Server listening on port %d", PORT);

    while (1) {
        int sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        ESP_LOGI(TAG, "Client connected");

        int offset = 0;
        while (1) {
            int len = recv(sock, rx_buffer + offset, RX_BUF_SIZE - offset - 1, 0);
            if (len <= 0) break;

            offset += len;
            rx_buffer[offset] = 0;

            char *newline = strchr(rx_buffer, '\n');
            if (newline) {
                *newline = 0;
                ESP_LOGI(TAG, "Received: %s", rx_buffer);
                send(sock, "OK\n", 3, 0);

                int remaining = offset - (newline - rx_buffer + 1);
                memmove(rx_buffer, newline + 1, remaining);
                offset = remaining;
            }

            if (offset >= RX_BUF_SIZE - 1) {
                rx_buffer[offset] = 0;
                ESP_LOGI(TAG, "Received (full buffer): %s", rx_buffer);
                send(sock, "OK\n", 3, 0);
                offset = 0;
            }
        }

        ESP_LOGI(TAG, "Client disconnected");
        close(sock);
    }
}

void app_main(void)
{
    nvs_flash_init();
    wifi_init();
    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
}
