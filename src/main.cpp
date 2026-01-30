/**
 * @file main.cpp
 * @brief Main application for ESP32-S3 GPS-based Stratum 1 NTP Server
 * 
 * Hardware Configuration:
 * - Board: Seeed Studios Xiao ESP32-S3
 * - GPS: Teyleten ATGM336M
 *   - UART: GPIO 43 (TX), GPIO 44 (RX)
 *   - PPS: GPIO 1
 * - Display: 0.91" 128x32 OLED I2C
 *   - I2C: GPIO 5 (SDA), GPIO 6 (SCL)
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "lwip/apps/sntp.h"

#include "gps_driver.h"
#include "ntp_server.h"
#include "oled_display.h"

static const char *TAG = "MAIN";

// WiFi credentials (should be configured via menuconfig or external config)
#ifndef CONFIG_WIFI_SSID
#define CONFIG_WIFI_SSID "YourSSID"
#endif

#ifndef CONFIG_WIFI_PASSWORD
#define CONFIG_WIFI_PASSWORD "YourPassword"
#endif

#define WIFI_SSID      CONFIG_WIFI_SSID
#define WIFI_PASSWORD  CONFIG_WIFI_PASSWORD

// Event group for WiFi status
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;
#define MAX_RETRY 5

static void event_handler(void* arg, esp_event_base_t event_base,
                         int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, WIFI_PASSWORD, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s", WIFI_SSID);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s", WIFI_SSID);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "ESP32-S3 GPS NTP Server Starting...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize OLED display
    ESP_LOGI(TAG, "Initializing OLED display...");
    oled_init();
    oled_display_text(0, "GPS NTP Server", 0);
    oled_display_text(1, "Starting...", 0);

    // Initialize GPS driver
    ESP_LOGI(TAG, "Initializing GPS module...");
    gps_init();
    oled_display_text(2, "GPS: Init", 0);

    // Initialize WiFi
    ESP_LOGI(TAG, "Initializing WiFi...");
    wifi_init_sta();
    oled_display_text(3, "WiFi: Connected", 0);

    // Initialize NTP server
    ESP_LOGI(TAG, "Starting NTP server...");
    ntp_server_init();

    // Main loop - update display with status
    char time_str[32];
    char gps_str[32];
    int loop_count = 0;
    
    while (1) {
        gps_data_t gps_data;
        if (gps_get_data(&gps_data)) {
            if (gps_data.fix_valid) {
                snprintf(gps_str, sizeof(gps_str), "GPS: Fix %dsat", gps_data.satellites);
                
                // Format time
                snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d UTC",
                        gps_data.hour, gps_data.minute, gps_data.second);
            } else {
                snprintf(gps_str, sizeof(gps_str), "GPS: No Fix");
                snprintf(time_str, sizeof(time_str), "Waiting...");
            }
        } else {
            snprintf(gps_str, sizeof(gps_str), "GPS: No Data");
            snprintf(time_str, sizeof(time_str), "---");
        }

        // Update display every second
        if (loop_count % 10 == 0) {
            oled_clear();
            oled_display_text(0, "GPS NTP Server", 0);
            oled_display_text(1, time_str, 0);
            oled_display_text(2, gps_str, 0);
            
            ntp_stats_t stats;
            ntp_get_stats(&stats);
            char ntp_str[32];
            snprintf(ntp_str, sizeof(ntp_str), "NTP Req: %lu", stats.request_count);
            oled_display_text(3, ntp_str, 0);
        }
        
        loop_count++;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
