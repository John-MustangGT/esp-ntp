#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_storage.h"
#include "wifi_provisioning.h"
#include "web_server.h"
#include "ntp_client.h"
#include "gps_nmea.h"
#include "pps_handler.h"
#include "oled_display.h"
#include "time_sync.h"

static const char *TAG = "MAIN";
static bool reboot_requested = false;

void trigger_reboot(void) {
    reboot_requested = true;
}

static void core0_task(void *pvParameters) {
    ESP_LOGI(TAG, "Core 0 task started");
    
    // Initialize OLED display
    oled_init();
    oled_display_status("ESP-NTP", "Initializing...", "");
    
    // Initialize NTP client
    nvs_config_t config;
    if (nvs_config_read(&config) == ESP_OK && config.ntp_server[0] != '\0') {
        ntp_client_init(config.ntp_server);
    } else {
        ntp_client_init(NTP_DEFAULT_SERVER);
    }
    
    // Start time synchronization orchestrator
    time_sync_init();
    time_sync_start();
    
    while (1) {
        // Core 0 main loop: NTP, web server, OLED display updates
        
        // Check for reboot request
        if (reboot_requested) {
            ESP_LOGI(TAG, "Reboot requested, waiting 3 seconds...");
            oled_display_status("Rebooting...", "", "");
            vTaskDelay(pdMS_TO_TICKS(3000));
            esp_restart();
        }
        
        // Update OLED with current status
        if (time_sync_is_synced()) {
            time_t now;
            time(&now);
            struct tm timeinfo;
            localtime_r(&now, &timeinfo);
            char strftime_buf[64];
            strftime(strftime_buf, sizeof(strftime_buf), "%H:%M:%S", &timeinfo);
            oled_display_time(strftime_buf);
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void core1_task(void *pvParameters) {
    ESP_LOGI(TAG, "Core 1 task started");
    
    // Initialize GPS UART
    gps_init();
    
    // Initialize PPS interrupt handler
    pps_init();
    
    // Start GPS parsing task (runs on Core 1)
    gps_start_task();
    
    while (1) {
        // Core 1 main loop: Monitor PPS and GPS status
        if (pps_is_active(2000)) {
            pps_edge_data_t pps_data;
            if (pps_get_last_edge(&pps_data)) {
                ESP_LOGD(TAG, "PPS pulse #%d at %lld us", pps_data.count, pps_data.timestamp_us);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "ESP-NTP Starting...");
    ESP_LOGI(TAG, "ESP-IDF Version: %s", esp_get_idf_version());

    // Initialize NVS
    ESP_ERROR_CHECK(nvs_storage_init());

    // Initialize event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize network interface
    ESP_ERROR_CHECK(esp_netif_init());

    // Check NVS configuration validity
    bool config_valid = nvs_config_is_valid();
    ESP_LOGI(TAG, "NVS Configuration: %s", config_valid ? "VALID" : "INVALID");

    if (!config_valid) {
        // Boot into AP mode for configuration
        ESP_LOGI(TAG, "Booting into AP mode for configuration");
        
        ESP_ERROR_CHECK(wifi_start_ap_mode());
        ESP_ERROR_CHECK(web_server_start(true));  // true = AP mode/config portal
        
        ESP_LOGI(TAG, "Configuration portal active");
        ESP_LOGI(TAG, "Connect to WiFi SSID: %s", WIFI_AP_SSID);
        ESP_LOGI(TAG, "Navigate to: http://192.168.4.1");
        
        // In AP mode, just run Core 0 task for web server
        xTaskCreatePinnedToCore(core0_task, "core0_task", 8192, NULL, 5, NULL, 0);
        
    } else {
        // Boot into normal operation mode
        ESP_LOGI(TAG, "Booting into station mode");
        
        // Read configuration
        nvs_config_t config;
        esp_err_t err = nvs_config_read(&config);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read config, restarting in AP mode");
            nvs_config_erase();
            esp_restart();
        }

        ESP_LOGI(TAG, "Connecting to WiFi SSID: %s", config.ssid);
        ESP_ERROR_CHECK(wifi_start_station_mode(config.ssid, config.password));
        
        // Wait for WiFi connection
        if (wifi_wait_for_connection(30000)) {
            ESP_LOGI(TAG, "WiFi connected successfully");
            
            // Start web server in normal mode
            ESP_ERROR_CHECK(web_server_start(false));  // false = normal mode
            
            // Start dual-core tasks
            xTaskCreatePinnedToCore(core0_task, "core0_task", 8192, NULL, 5, NULL, 0);
            xTaskCreatePinnedToCore(core1_task, "core1_task", 8192, NULL, 5, NULL, 1);
            
            ESP_LOGI(TAG, "System initialization complete");
            
        } else {
            ESP_LOGE(TAG, "Failed to connect to WiFi, restarting in AP mode");
            nvs_config_erase();
            esp_restart();
        }
    }
}
